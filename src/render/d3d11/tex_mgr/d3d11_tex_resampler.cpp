// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <SpecialK/stdafx.h>

#ifdef  __SK_SUBSYSTEM__
#undef  __SK_SUBSYSTEM__
#endif
#define __SK_SUBSYSTEM__ L"D3D11Sampl"

#include <SpecialK/render/d3d11/d3d11_core.h>

static       HANDLE hResampleWork       = INVALID_HANDLE_VALUE;
static unsigned int dispatch_thread_idx = 0;

int SK_D3D11_TexStreamPriority = 1; // 0=Low  (Pop-In),
                                    // 1=Balanced,
                                    // 2=High (No Pop-In)

struct resample_dispatch_s
{
  void delayInit (void)
  {
    hResampleWork =
      SK_CreateEvent ( nullptr, FALSE,
                                FALSE,
          SK_FormatStringW ( LR"(Local\SK_DX11TextureResample_Dispatch_%x)",
                               GetCurrentProcessId () ).c_str ()
      );

    SYSTEM_INFO        sysinfo = { };
    SK_GetSystemInfo (&sysinfo);

    max_workers =
      std::max (1UL, sysinfo.dwNumberOfProcessors - 1);

    PROCESSOR_NUMBER                                 pnum = { };
    GetThreadIdealProcessorEx (GetCurrentThread (), &pnum);

    dispatch_thread_idx =
           ( pnum.Group + pnum.Number );
  }

  bool postJob (resample_job_s job)
  {
    job.time.received =
      SK_QueryPerf ().QuadPart;

                           job.texture->AddRef ();
    waiting_textures.push (job);

    InterlockedIncrement  (&stats.textures_waiting);

    SK_RunOnce (delayInit ());

    bool ret = false;

    if (InterlockedIncrement (&active_workers) <= max_workers)
    {
      SK_Thread_Create ( [](LPVOID pDispatchBase) ->
      DWORD
      {
        auto pResampler =
          (resample_dispatch_s *)pDispatchBase;

        SetThreadPriority ( SK_GetCurrentThread (),
                            THREAD_MODE_BACKGROUND_BEGIN );

        static volatile LONG thread_num = 0;

        uintptr_t thread_idx = ReadAcquire (&thread_num);
        uintptr_t logic_idx  = 0;

        // There will be an odd-man-out in any SMT implementation since the
        //   SwapChain thread has an ideal _logical_ CPU core.
        bool         disjoint     = false;


        InterlockedIncrement (&thread_num);


        if (thread_idx == dispatch_thread_idx)
        {
          disjoint = true;
          thread_idx++;
          InterlockedIncrement (&thread_num);
        }


        for ( auto& it : SK_CPU_GetLogicalCorePairs () )
        {
          if ( it & ( (uintptr_t)1 << thread_idx ) ) break;
          else                       ++logic_idx;
        }

        const ULONG_PTR logic_mask =
          SK_CPU_GetLogicalCorePairs ()[logic_idx];


        SK_TLS *pTLS =
          SK_TLS_Bottom ();

        SK_SetThreadIdealProcessor ( SK_GetCurrentThread (),
                                       sk::narrow_cast <DWORD> (thread_idx) );
        if (! disjoint)
        {
          SetThreadAffinityMask ( SK_GetCurrentThread (), logic_mask );
        }

        std::wstring desc = L"[SK] D3D11 Texture Resampling ";

  const int logical_siblings = CountSetBits (logic_mask);
        if (logical_siblings > 1 && (! disjoint))
        {
          desc += L"HyperThread < ";
          for ( int i = 0,
                    j = 0;
                    i < SK_GetBitness ();
                  ++i )
          {
            if ( (logic_mask & ( static_cast <uintptr_t> (1) << i ) ) )
            {
              desc += std::to_wstring (i);

              if ( ++j < logical_siblings ) desc += L",";
            }
          }
          desc += L" >";
        }

        else
          desc += SK_FormatStringW (L"Thread %lu", thread_idx);


        SetCurrentThreadDescription (desc.c_str ());

        SK_ScopedBool decl_tex_scope (
          SK_D3D11_DeclareTexInjectScope ()
        );

        auto* task =
          SK_MMCS_GetTaskForThreadIDEx ( SK_Thread_GetCurrentId (),
                                           SK_WideCharToUTF8 (desc).c_str (),
                                             "Playback",
                                             "DisplayPostProcessing" );

        if ( task        != nullptr                &&
             task->dwTid == SK_Thread_GetCurrentId () )
        {
          task->setPriority (AVRT_PRIORITY_HIGH);
        }

        else
        {
          SetThreadPriorityBoost ( SK_GetCurrentThread (),
                                   TRUE );
          SetThreadPriority (      SK_GetCurrentThread (),
                                   THREAD_PRIORITY_TIME_CRITICAL );
        }

        do
        {
          resample_job_s job = { };

          while (pResampler->waiting_textures.try_pop (job))
          {
            job.time.started = SK_QueryPerf ().QuadPart;

            InterlockedDecrement (&pResampler->stats.textures_waiting);
            InterlockedIncrement (&pResampler->stats.textures_resampling);

            DirectX::ScratchImage* pNewImg = nullptr;

            const HRESULT hr =
              SK_D3D11_MipmapCacheTexture2DEx ( *job.data,
                                                 job.crc32c,
                                                 job.texture,
                                                &pNewImg,
                                                 pTLS );

            delete job.data;

            if (FAILED (hr))
            {
              dll_log->Log (L"Resampler failure (crc32c=%x)", job.crc32c);
              InterlockedIncrement (&pResampler->stats.error_count);
              job.texture->Release ();
            }

            else
            {
              job.time.finished = SK_QueryPerf ().QuadPart;
              job.data          = pNewImg;

              pResampler->finished_textures.push (job);
            }

            InterlockedDecrement (&pResampler->stats.textures_resampling);
          }

          ///if (task != nullptr && task->hTask > 0)
          ///    task->disassociateWithTask ();

          if (SK_WaitForSingleObject (hResampleWork, 666UL) != WAIT_OBJECT_0)
          {
            if ( task        == nullptr                ||
                 task->dwTid != SK_Thread_GetCurrentId () )
            {
              SetThreadPriorityBoost ( SK_GetCurrentThread (),
                                       FALSE );
              SetThreadPriority (      SK_GetCurrentThread (),
                                       THREAD_PRIORITY_LOWEST );
            }

            SK_WaitForSingleObject (hResampleWork, INFINITE);
          }

          ///if (task != nullptr && task->hTask > 0)
          ///    task->reassociateWithTask ();
        } while (ReadAcquire (&__SK_DLL_Ending) == 0);

        InterlockedDecrement (&pResampler->active_workers);

        SK_Thread_CloseSelf ();

        return 0;
      }, (LPVOID)this );

      ret = true;
    }

    SetEvent (hResampleWork);

    return ret;
  };


  bool processFinished ( ID3D11Device        *pDev,
                         ID3D11DeviceContext *pDevCtx,
                         SK_TLS              *pTLS = SK_TLS_Bottom () )
  {
    size_t MAX_TEXTURE_UPLOADS_PER_FRAME;
    int    MAX_UPLOAD_TIME_PER_FRAME_IN_MS;

    switch    (SK_D3D11_TexStreamPriority)
    {
      case 0:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 3UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   =
          static_cast <size_t> (ReadAcquire (&stats.textures_waiting) / 7) + 1;
        break;

      case 1:
      default:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 13UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   =
          static_cast <size_t> (ReadAcquire (&stats.textures_waiting) / 4) + 1;
        break;

      case 2:
        MAX_UPLOAD_TIME_PER_FRAME_IN_MS = 27UL;
        MAX_TEXTURE_UPLOADS_PER_FRAME   =
          static_cast <size_t> (ReadAcquire (&stats.textures_waiting) / 2) + 1;
    }

          size_t   uploaded   = 0;
    const uint64_t start_tick = SK::ControlPanel::current_tick;//SK_QueryPerf ().QuadPart;
    const uint64_t deadline   = start_tick + MAX_UPLOAD_TIME_PER_FRAME_IN_MS * SK_PerfTicksPerMs;

    SK_ScopedBool auto_bool_mem (&pTLS->imgui->drawing);
                                  pTLS->imgui->drawing = true;
    SK_ScopedBool decl_tex_scope (
      SK_D3D11_DeclareTexInjectScope (pTLS)
    );

    bool processed = false;

    auto& textures =
      SK_D3D11_Textures;

    //
    // Finish Resampled Texture Uploads (discard if texture is no longer live)
    //
    while (! finished_textures.empty ())
    {
      resample_job_s finished = { };

      if ( pDev    != nullptr &&
           pDevCtx != nullptr    )
      {
        if (finished_textures.try_pop (finished))
        {
          // Due to cache purging behavior and the fact that some crazy games issue back-to-back loads of the same resource,
          //   we need to test the cache health prior to trying to service this request.
          if ( ( ( finished.time.started > textures->LastPurge_2D ) ||
                 ( SK_D3D11_TextureIsCached (finished.texture) )  ) &&
                finished.data         != nullptr                    &&
                finished.texture      != nullptr )
          {
            SK_ComPtr <ID3D11Texture2D> pTempTex;

            const HRESULT ret =
              DirectX::CreateTexture (
                pDev, finished.data->GetImages   (), finished.data->GetImageCount (),
                      finished.data->GetMetadata (), (ID3D11Resource **)&pTempTex);

            if (SUCCEEDED (ret))
            {

              ///D3D11_TEXTURE2D_DESC        texDesc = { };
              ///finished.texture->GetDesc (&texDesc);
              ///
              ///pDevCtx->SetResourceMinLOD (finished.texture, (FLOAT)(texDesc.MipLevels - 1));

              pDevCtx->CopyResource (finished.texture, pTempTex);

              uploaded++;

              InterlockedIncrement (&stats.textures_finished);

              // Various wait-time statistics;  the job queue is HyperThreading aware and helps reduce contention on
              //                                  the list of finished textures ( Which we need to service from the
              //                                                                   game's original calling thread ).
              const uint64_t wait_in_queue = ( finished.time.started      - finished.time.received );
              const uint64_t work_time     = ( finished.time.finished     - finished.time.started  );
              const uint64_t wait_finished = ( SK_CurrentPerf ().QuadPart - finished.time.finished );

              SK_LOG1 ( (L"ReSample Job %4lu (hash=%08x {%4lux%#4lu}) => %9.3f ms TOTAL :: ( %9.3f ms pre-proc, "
                                                                                           L"%9.3f ms work queue, "
                                                                                           L"%9.3f ms resampling, "
                                                                                           L"%9.3f ms finish queue )",
                           ReadAcquire (&stats.textures_finished), finished.crc32c,
                           finished.data->GetMetadata ().width,    finished.data->GetMetadata ().height,
                         ( (long double)SK_CurrentPerf ().QuadPart - (long double)finished.time.received +
                           (long double)finished.time.preprocess ) / (long double)SK_PerfTicksPerMs,
                           (long double)finished.time.preprocess   / (long double)SK_PerfTicksPerMs,
                           (long double)wait_in_queue              / (long double)SK_PerfTicksPerMs,
                           (long double)work_time                  / (long double)SK_PerfTicksPerMs,
                           (long double)wait_finished              / (long double)SK_PerfTicksPerMs ),
                       L"DX11TexMgr"  );
            }

            else
              InterlockedIncrement (&stats.error_count);
          }

          else
          {
            SK_LOG0 ( (L"Texture (%x) was loaded too late, discarding...", finished.crc32c),
                       L"DX11TexMgr" );

            InterlockedIncrement (&stats.textures_too_late);
          }


          if (finished.data != nullptr)
          {
            //delete finished.data;
                   finished.data = nullptr;
          }



          IUnknown_AtomicRelease ((void **)&finished.texture);

          processed = true;


          if ( uploaded >= MAX_TEXTURE_UPLOADS_PER_FRAME ||
               deadline <  (uint64_t)SK_QueryPerf ().QuadPart )  break;
        }

        else
          break;
      }

      else
        break;
    }

    return processed;
  };


  struct stats_s
  {
    ~stats_s (void) ///
    {
      if (hResampleWork != INVALID_HANDLE_VALUE)
      {
        SK_CloseHandle (hResampleWork);
                        hResampleWork = INVALID_HANDLE_VALUE;
      }
    }

    volatile LONG textures_resampled    = 0L;
    volatile LONG textures_compressed   = 0L;
    volatile LONG textures_decompressed = 0L;

    volatile LONG textures_waiting      = 0L;
    volatile LONG textures_resampling   = 0L;
    volatile LONG textures_too_late     = 0L;
    volatile LONG textures_finished     = 0L;

    volatile LONG error_count           = 0L;
  } stats;

           LONG max_workers    = 0;
  volatile LONG active_workers = 0;

  concurrency::concurrent_queue <resample_job_s> waiting_textures;
  concurrency::concurrent_queue <resample_job_s> finished_textures;
};

SK_LazyGlobal <resample_dispatch_s> SK_D3D11_TextureResampler;


LONG
SK_D3D11_Resampler_GetActiveJobCount (void)
{
  return
    ReadAcquire (&SK_D3D11_TextureResampler->stats.textures_resampling);
}

LONG
SK_D3D11_Resampler_GetWaitingJobCount (void)
{
  return
    ReadAcquire (&SK_D3D11_TextureResampler->stats.textures_waiting);
}

LONG
SK_D3D11_Resampler_GetRetiredCount (void)
{
  return
    ReadAcquire (&SK_D3D11_TextureResampler->stats.textures_resampled);
}

LONG
SK_D3D11_Resampler_GetErrorCount (void)
{
  return
    ReadAcquire (&SK_D3D11_TextureResampler->stats.error_count);
}

bool
SK_D3D11_Resampler_PostJob (resample_job_s job)
{
  return
    SK_D3D11_TextureResampler->postJob (job);
}

bool
SK_D3D11_Resampler_ProcessFinished ( ID3D11Device        *pDev,
                                     ID3D11DeviceContext *pDevCtx,
                                     SK_TLS              *pTLS )
{
  return
    SK_D3D11_TextureResampler->processFinished (pDev, pDevCtx, pTLS);
}
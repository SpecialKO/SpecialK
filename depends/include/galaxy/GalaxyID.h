 #ifndef GALAXY_GALAXY_ID_H
 #define GALAXY_GALAXY_ID_H
  
 #include "stdint.h"
 #include <assert.h>
  
 namespace galaxy
 {
     namespace api
     {
 #pragma pack( push, 1 )
  
         class GalaxyID
         {
         public:
  
             enum IDType {
                 ID_TYPE_UNASSIGNED,
                 ID_TYPE_LOBBY,
                 ID_TYPE_USER
             };
  
             static const uint64_t UNASSIGNED_VALUE = 0;
  
             static GalaxyID FromRealID(IDType type, uint64_t value)
             {
                 assert(type != ID_TYPE_UNASSIGNED);
                 assert(value != UNASSIGNED_VALUE);
                 assert(static_cast<IDType>(value >> 56) == ID_TYPE_UNASSIGNED);
                 return GalaxyID(static_cast<uint64_t>(type) << 56 | value);
             }
  
             GalaxyID(void) : value(UNASSIGNED_VALUE)
             {
             }
  
             GalaxyID(uint64_t _value) : value(_value)
             {
             }
  
             GalaxyID(const GalaxyID& galaxyID) : value(galaxyID.value)
             {
             }
  
             GalaxyID& operator=(const GalaxyID& other)
             {
                 value = other.value;
                 return *this;
             }
  
             bool operator<(const GalaxyID& other) const
             {
                 assert(IsValid() && other.IsValid());
                 return value < other.value;
             }
  
             bool operator==(const GalaxyID& other) const
             {
                 return value == other.value;
             }
  
             bool operator!=(const GalaxyID& other) const
             {
                 return !(*this == other);
             }
  
             bool IsValid() const
             {
                 return value != UNASSIGNED_VALUE;
             }
  
             uint64_t ToUint64() const
             {
                 return value;
             }
  
             uint64_t GetRealID() const
             {
                 return value & 0xffffffffffffff;
             }
  
             IDType GetIDType() const
             {
                 return static_cast<IDType>(value >> 56);
             }
  
         private:
  
             uint64_t value; 
         };
  
 #pragma pack( pop )
  
     }
 }
  
 #endif
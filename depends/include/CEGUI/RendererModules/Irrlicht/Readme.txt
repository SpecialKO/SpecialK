CEGUI IrrlichtRenderer

For license information read the remarks in the file:
"include/RendererModule/Irrlicht/CEGUIIrrlichtRendererDef.h"

Use the method "bool IrrlichtRenderer::OnEvent(SEvent e)" to push events into
the gui system.

Example:

-------------------------

// create a renderer which uses the irrlicht filesystem to load data
renderer=new CEGUI::IrrlichtRenderer(device,true);

// create the gui
new CEGUI::System(renderer);

// draw everything
while(device->run())
{
    // draw only if the window is active
    if (device->isWindowActive())
    {
        driver->beginScene(true, true, video::SColor(150,50,50,50));
        //draw scene
        smgr->drawAll();
        // draw gui
        CEGUI::System::getSingleton().renderGUI();
        driver->endScene();
    }
}

--------------------------

// irrlicht event handler function
bool CEGuiTest::OnEvent(SEvent event)
{
    if(irrlichtrenderer!=0)
        return irrlichtrenderer->injectEvent(event);
    else
        return false;
}

--------------------------

Changes:
22/12/2004
- added IrrlichResourceProvider
- added IrrlichtEventPusher

6/1/2005
- IrrlichtResourceProvider is now Optional

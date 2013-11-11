
// Part of the Soundplane client software by Madrona Labs.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "SoundplaneZoneView.h"

SoundplaneZoneView::SoundplaneZoneView() :
	mpModel(nullptr)
{
#if JUCE_IOS
	// (On the iPhone, choose a format without a depth buffer)
	setPixelFormat (OpenGLPixelFormat (8, 8, 0, 0));
#endif
	setInterceptsMouseClicks (false, false);	
	MLWidget::setComponent(this);
	mGLContext.setRenderer (this);
	mGLContext.setComponentPaintingEnabled (false);
}

SoundplaneZoneView::~SoundplaneZoneView()
{
    stopTimer();
	mGLContext.detach();
}

void SoundplaneZoneView::setModel(SoundplaneModel* m)
{
	mpModel = m;
    startTimer(10);
}

void SoundplaneZoneView::timerCallback()
{
    mGLContext.triggerRepaint();
}

void SoundplaneZoneView::newOpenGLContextCreated()
{
#if ! JUCE_IOS
	// (no need to call makeCurrentContextActive(), as that will have
	// been done for us before the method call).
	glClearColor (0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth (1.0);

	glDisable (GL_DEPTH_TEST);
	glEnable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glShadeModel (GL_SMOOTH);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glPixelStorei (GL_UNPACK_ALIGNMENT, 4);

	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	glHint (GL_POINT_SMOOTH_HINT, GL_NICEST);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

void SoundplaneZoneView::openGLContextClosing()
{
}

void SoundplaneZoneView::mouseDrag (const MouseEvent& e)
{
}

// temporary, ugly
// TODO more 2d OpenGL drawing helpers
void SoundplaneZoneView::drawDot(Vec2 pos, float r)
{
	int steps = 16;
    
	float x = pos.x();
	float y = pos.y();
    
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(x, y);
	for(int i=0; i<=steps; ++i)
	{
		float theta = kMLTwoPi * (float)i / (float)steps;
		float rx = r*cosf(theta);
		float ry = r*sinf(theta);
		glVertex3f(x + rx, y + ry, 0.f);
	}
	glEnd();
}

void SoundplaneZoneView::renderGrid()
{
    int viewW = getBackingLayerWidth();
    int viewH = getBackingLayerHeight();
    MLGL::orthoView2(viewW, viewH);

	int gridWidth = 30; // Soundplane A TODO get from tracker
	int gridHeight = 5;
   // int margin = viewW / 50;
	MLRange xRange(0, gridWidth, 1, viewW);
	MLRange yRange(0, gridHeight, 1, viewH);
    
	// draw thin lines at key grid
	Vec4 lineColor;
	Vec4 darkBlue(0.3f, 0.3f, 0.5f, 1.f);
	Vec4 gray(0.6f, 0.6f, 0.6f, 1.f);
	lineColor = gray;

	// horiz lines
	glColor4fv(&lineColor[0]);
	for(int j=0; j<=gridHeight; ++j)
	{
		glBegin(GL_LINE_STRIP);
		for(int i=0; i<=gridWidth; ++i)
		{
			float x = xRange.convert(i);
			float y = yRange.convert(j);
			glVertex2f(x, y);
		}
		glEnd();
	}
	// vert lines
	for(int i=0; i<=gridWidth; ++i)
	{
		glBegin(GL_LINE_STRIP);
		for(int j=0; j<=gridHeight; ++j)
		{
			float x = xRange.convert(i);
			float y = yRange.convert(j);
			glVertex2f(x, y);
		}
		glEnd();
	}
	
	// draw dots
    float r = viewH / 80.;
	for(int i=0; i<=gridWidth; ++i)
	{
		float x = xRange.convert(i + 0.5);
		float y = yRange.convert(2.5);
		int k = i%12;
		if(k == 0)
		{
			float d = viewH / 50;
            Vec4 dotColor(0.6f, 0.6f, 0.6f, 1.f);
            glColor4fv(&dotColor[0]);
            drawDot(Vec2(x, y - d), r);
			drawDot(Vec2(x, y + d), r);
		}
		if((k == 3)||(k == 5)||(k == 7)||(k == 9))
		{
            Vec4 dotColor(0.6f, 0.6f, 0.6f, 1.f);
            glColor4fv(&dotColor[0]);
            drawDot(Vec2(x, y), r);
		}
	}
}

void SoundplaneZoneView::renderZones()
{
	if (!mpModel) return;
    const ScopedLock lock(*(mpModel->getZoneLock()));
    const std::vector<ZonePtr>& zoneList = mpModel->getZones();

    int viewW = getBackingLayerWidth();
    int viewH = getBackingLayerHeight();
	// float viewAspect = (float)viewW / (float)viewH;
    
	int gridWidth = 30; // Soundplane A TODO get from tracker
	int gridHeight = 5;
    int lineWidth = viewW / 200;
    int thinLineWidth = viewW / 400;
    // int margin = lineWidth*2;
    
    // put origin in lower left. 
    MLGL::orthoView2(viewW, viewH);
    MLRange xRange(0, gridWidth, 1, viewW);
	MLRange yRange(0, gridHeight, 1, viewH);

	Vec4 lineColor;
	Vec4 darkBlue(0.3f, 0.3f, 0.5f, 1.f);
	Vec4 gray(0.6f, 0.6f, 0.6f, 1.f);
	Vec4 lightGray(0.9f, 0.9f, 0.9f, 1.f);
	Vec4 blue2(0.1f, 0.1f, 0.5f, 1.f);
    float smallDotSize = 4.f;
    
   // float strokeWidth = viewW / 100;
    
    
    std::vector<ZonePtr>::const_iterator it;

    for(it = zoneList.begin(); it != zoneList.end(); ++it)
    {
        const Zone& zone = **it;
        
        int t = zone.getType();
        MLRect zr = zone.getBounds();
        const char * name = zone.getName().c_str();
        
        // affine transforms TODO for better syntax: MLRect zrd = zr.xform(gridToView);
        
        MLRect zoneRectInView(xRange.convert(zr.x()), yRange.convert(zr.y()), xRange.convert(zr.width()), yRange.convert(zr.height()));
        zoneRectInView.shrink(lineWidth);
        Vec4 zoneStroke(MLGL::getIndicatorColor(t));
        Vec4 zoneFill(zoneStroke);
        zoneFill[3] = 0.1f;
        Vec4 activeFill(zoneStroke);
        activeFill[3] = 0.25f;
        Vec4 dotFill(zoneStroke);
        dotFill[3] = 0.5f;
        
        // draw box common to all kinds of zones
        glColor4fv(&zoneFill[0]);
        MLGL::fillRect(zoneRectInView);
        glColor4fv(&zoneStroke[0]);
        glLineWidth(lineWidth);
        MLGL::strokeRect(zoneRectInView);
        glLineWidth(1);
        // draw name
        // all these rect calculations read upside-down here because view origin is at bottom
        MLGL::drawTextAt(zoneRectInView.left() + lineWidth, zoneRectInView.top() + lineWidth, 0.f, name);
        
        // draw any zone-specific things
        float x, y, z;
        switch(t)
        {
            case kNoteRow:
                for(int i = 0; i < kSoundplaneMaxTouches; ++i)
                {
                    const ZoneTouch& uTouch = zone.getTouch(i);
                    const ZoneTouch& touch = zone.touchToKeyPos(uTouch);
                    if(touch.isActive())
                    {
                        glColor4fv(&dotFill[0]);
                        float dx = xRange(touch.pos.x());
                        float dy = yRange(touch.pos.y());
                        float dz = touch.pos.z();
                        drawDot(Vec2(dx, dy), smallDotSize + dz*smallDotSize);
                    }
                }
                break;
                
            case kControllerX:
                x = xRange(zone.getXKeyPos());
                glColor4fv(&zoneStroke[0]);
                glLineWidth(thinLineWidth);
                MLGL::strokeRect(MLRect(x, zoneRectInView.top(), 0., zoneRectInView.height()));
                glColor4fv(&activeFill[0]);
                MLGL::fillRect(MLRect(zoneRectInView.left(), zoneRectInView.top(), x - zoneRectInView.left(), zoneRectInView.height()));
                break;
                
            case kControllerY:
                y = yRange(zone.getYKeyPos());
                glColor4fv(&zoneStroke[0]);
                glLineWidth(thinLineWidth);                
                MLGL::strokeRect(MLRect(zoneRectInView.left(), y, zoneRectInView.width(), 0.));
                glColor4fv(&activeFill[0]);
                MLGL::fillRect(MLRect(zoneRectInView.left(), zoneRectInView.top(), zoneRectInView.width(), y - zoneRectInView.top()));
                break;
                
            case kControllerXY:
                x = xRange(zone.getXKeyPos());
                y = yRange(zone.getYKeyPos());
                glColor4fv(&zoneStroke[0]);
                glLineWidth(thinLineWidth);
                // cross-hairs centered on dot
                MLGL::strokeRect(MLRect(x, zoneRectInView.top(), 0., zoneRectInView.height()));
                MLGL::strokeRect(MLRect(zoneRectInView.left(), y, zoneRectInView.width(), 0.));                
                glColor4fv(&dotFill[0]);
                drawDot(Vec2(x, y), smallDotSize);
                break;            
        }
    }
      
    /*
     types
     kNoteRow = 0,
     kControllerHorizontal = 1,
     kControllerVertical = 2,
     kControllerXY = 3,
     kToggleRow = 4
     
    ZoneType mType;
    MLRect mRect;
    int mStartNote;
    int mControllerNumber;
    int mChannel;
    std::string mName;
    */
    
}

void SoundplaneZoneView::renderOpenGL()
{
	if (!mpModel) return;
    int backW = getBackingLayerWidth();
    int backH = getBackingLayerHeight();
    if(!mGLContext.isAttached()) return;
    ScopedPointer<LowLevelGraphicsContext> glRenderer(createOpenGLGraphicsContext (mGLContext, backW, backH));
    if (glRenderer != nullptr)
    {
        Graphics g (glRenderer);
        const Colour c = findColour(MLLookAndFeel::backgroundColor);
        OpenGLHelpers::clear (c);
        glEnable(GL_BLEND);
        glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        renderGrid();
        renderZones();
    }
}

// GL views need to attach to their components here, because on creation
// the component might not be visible and can't be attached to.
void SoundplaneZoneView::resizeWidget(const MLRect& b, const int u)
{
    MLWidget::resizeWidget(b, u);
    mGLContext.attachTo (*MLWidget::getComponent());
}





	
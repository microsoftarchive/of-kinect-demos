#pragma once

#include "ofMain.h"

namespace Kinect = WindowsPreview::Kinect;

using namespace Kinect;
namespace WFC = Windows::Foundation::Collections;

struct MyJoint{
	ofPoint position;
	ofColor color;
};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		Kinect::KinectSensor^				sensor;
		Kinect::DepthFrameReader^			depthReader;
		Kinect::ColorFrameReader^			colorReader;
		Kinect::BodyFrameReader^			bodyReader;
		Kinect::CoordinateMapper^			coordMapper;
		WFC::IVector<Kinect::Body^>^		bdyArray;
		Platform::Array<unsigned char>^		colorDataFrame;
		FrameDescription^					depthFrameDescription;

		ofTexture							txtColor;
		ofImage								imgDepth;

		int zoomFactor = 1;
		int mode = 0;

		vector<MyJoint> myJoints;

		float rx, ry;
		float lx, ly;
		float hx, hy;
		float htx, hty;
		float thrx, thry;
		ofColor colorLeftHand=ofColor::white;
		bool lassoFun = false;

		void UpdateColorFrame();
		void UpdateDepthFrame();
		void UpdateBodyFrameColor();
		void UpdateBodyFrameDepth();

		// rustic version :-)
		void UpdateDepthFrameOld();
		
};

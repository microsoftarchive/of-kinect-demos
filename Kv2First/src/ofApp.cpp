#include "ofApp.h"

using namespace WindowsPreview::Kinect;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Storage::Streams;



static byte* GetPointerToPixelData(IBuffer^ pBuffer)
{
	// Query the IBufferByteAccess interface.
	Microsoft::WRL::ComPtr<IBufferByteAccess> spBufferByteAccess;
	reinterpret_cast<IInspectable*>(pBuffer)->QueryInterface(IID_PPV_ARGS(&spBufferByteAccess));

	// Retrieve the buffer data.
	byte* pixels = nullptr;
	spBufferByteAccess->Buffer(&pixels);

	return pixels;
}
//--------------------------------------------------------------

void ofApp::setup(){
	
	// get the kinectSensor object
	sensor = KinectSensor::GetDefault();
	
	
	// open the sensor
	sensor->Open();
	

	//open the readers
	depthReader = sensor->DepthFrameSource->OpenReader();
	colorReader = sensor->ColorFrameSource->OpenReader();
	bodyReader  = sensor->BodyFrameSource->OpenReader();


	//Frame description for the color..
	FrameDescription^ colorFrameDescription = sensor->ColorFrameSource->FrameDescription;
	txtColor.allocate(colorFrameDescription->Width, colorFrameDescription->Height, GL_RGBA);

	//Allocate Color data frame array
	colorDataFrame = ref new Platform::Array<unsigned char>(colorFrameDescription->Width * colorFrameDescription->Height * 4);


	//framedescription for the depth..
	depthFrameDescription = sensor->DepthFrameSource->FrameDescription;
	//allocate the OFimage for depth
	imgDepth.allocate(depthFrameDescription->Width, depthFrameDescription->Height, ofImageType::OF_IMAGE_GRAYSCALE);

	
	//the body array of bodies in the bodyFrame
	bdyArray = ref new Platform::Collections::Vector <Kinect::Body^>(sensor->BodyFrameSource->BodyCount);
	
	//sensor-screen mapper
	coordMapper = sensor->CoordinateMapper;
	
}
//--------------------------------------------------------------

void ofApp::update(){
	if ((mode == 0) || (mode == 2))
		UpdateColorFrame();
	
	if ((mode == 1) || (mode == 3))
		UpdateDepthFrame();
	
	if ((mode == 2))
		UpdateBodyFrameColor();

	if ((mode == 3))
		UpdateBodyFrameDepth();
}
//--------------------------------------------------------------

void ofApp::draw(){

	if (!sensor->IsAvailable)
	{
		ofDrawBitmapString("No Kinect found :-(", 100, 100);
		return;
	}
	
	if ((mode == 0) || (mode == 2))
		txtColor.draw(0, 0);
	
	if ((mode == 1) || (mode == 3))
		imgDepth.draw(0, 0, depthFrameDescription->Width*zoomFactor, depthFrameDescription->Height*zoomFactor);

	if ((mode == 2) || (mode == 3))
	{
		ofCircle(rx*zoomFactor, ry*zoomFactor, 20);
		ofSetColor(colorLeftHand);
		ofCircle(lx*zoomFactor, ly*zoomFactor, 20);
		ofSetColor(ofColor::white);
		ofCircle(hx*zoomFactor, hy*zoomFactor, 50);

		ofSetColor(ofColor::blue);
		ofCircle(thrx*zoomFactor, thry*zoomFactor, 5);
		ofCircle(htx*zoomFactor, hty*zoomFactor, 5);
		ofSetColor(ofColor::white);
	}

}
//--------------------------------------------------------------

void ofApp::UpdateDepthFrame(){
	DepthFrame^ frame = depthReader->AcquireLatestFrame();
	if (nullptr != frame)
	{
		int nMinDepth = frame->DepthFrameSource->DepthMinReliableDistance;
		int nMaxDepth = frame->DepthFrameSource->DepthMaxReliableDistance;
		
		IBuffer^ pBuffer = frame->LockImageBuffer();
		UINT16* pSrc = reinterpret_cast<UINT16*>(GetPointerToPixelData(pBuffer));
		const UINT16* pBufferEnd = pSrc + (depthFrameDescription->Width * depthFrameDescription->Height);
			
		ofColor c;
		for (unsigned int y = 0; y < depthFrameDescription->Height; y++)
		{
			unsigned int x = 0;

			while (x < depthFrameDescription->Width)
			{
				USHORT depth = pSrc[x];
				BYTE intensity = static_cast<BYTE>((depth >= nMinDepth) && (depth <= nMaxDepth) ? (depth % 256) : 0);
				c.r = intensity;
				c.g = intensity;
				c.b = intensity;

				imgDepth.setColor(x, y, c);

				x++;
			}
			pSrc += depthFrameDescription->Width ;
		}

		imgDepth.update();
	}
}
//--------------------------------------------------------------

void ofApp::UpdateDepthFrameOld(){
	DepthFrame^ frame = depthReader->AcquireLatestFrame();
	if (nullptr != frame)
	{
		int nMinDepth = frame->DepthFrameSource->DepthMinReliableDistance;
		int nMaxDepth = frame->DepthFrameSource->DepthMaxReliableDistance;

		IBuffer^ pBuffer = frame->LockImageBuffer();
		UINT16* pSrc = reinterpret_cast<UINT16*>(GetPointerToPixelData(pBuffer));
		const UINT16* pBufferEnd = pSrc + (depthFrameDescription->Width * depthFrameDescription->Height);

		int i = 0;
		int x = 0;
		int y = 0;
		ofColor c;

		while (pSrc < pBufferEnd)
		{
			x = i;
			i++;

			if (i >= depthFrameDescription->Width)
			{
				y++;
				x = 0;
				i = 0;
			}
			USHORT depth = *pSrc;
			BYTE intensity = static_cast<BYTE>((depth >= nMinDepth) && (depth <= nMaxDepth) ? (depth % 256) : 0);
			c.r = intensity;
			c.g = intensity;
			c.b = intensity;

			imgDepth.setColor(x, y, c);
			++pSrc;
		}


		imgDepth.update();
	}
}
//--------------------------------------------------------------

void ofApp::UpdateColorFrame(){
	//update the color Frame
	ColorFrame^ frame = colorReader->AcquireLatestFrame();
	if (nullptr != frame)
	{
		frame->CopyConvertedFrameDataToArray(colorDataFrame, ColorImageFormat::Rgba);
		txtColor.loadData(colorDataFrame->Data, frame->FrameDescription->Width, frame->FrameDescription->Height, GL_RGBA);
	}

}
//--------------------------------------------------------------

void ofApp::UpdateBodyFrameDepth()
{
	BodyFrame^ bdyFrame = bodyReader->AcquireLatestFrame();
	if (nullptr != bdyFrame)
	{
		bdyFrame->GetAndRefreshBodyData(bdyArray);
		for each (Body^ body in bdyArray)
		{
			if (body->IsTracked)
			{
				WFC::IMapView<JointType, Joint>^ joints = body->Joints;
				for each(auto joint in joints)
				{
					if (joint->Key == JointType::HandRight)
					{
						CameraSpacePoint position = joint->Value.Position;
						DepthSpacePoint depthSpacePoint = coordMapper->MapCameraPointToDepthSpace(position);
						//body
						rx = depthSpacePoint.X;
						ry = depthSpacePoint.Y;
						//ColorSpacePoint colorSpacePoint = coordMapper->MapCameraPointToColorSpace(position);

					}
					else if (joint->Key == JointType::HandLeft)
					{
						CameraSpacePoint position = joint->Value.Position;
						DepthSpacePoint depthSpacePoint = coordMapper->MapCameraPointToDepthSpace(position);
						lx = depthSpacePoint.X;
						ly = depthSpacePoint.Y;
					}

					else if (joint->Key == JointType::Head)
					{
						CameraSpacePoint position = joint->Value.Position;
						DepthSpacePoint depthSpacePoint = coordMapper->MapCameraPointToDepthSpace(position);
						hx = depthSpacePoint.X;
						hy = depthSpacePoint.Y;
					}
				}
			}
		}
	}

}
//--------------------------------------------------------------

void ofApp::UpdateBodyFrameColor()
{
	BodyFrame^ bdyFrame = bodyReader->AcquireLatestFrame();
	if (nullptr != bdyFrame)
	{
		bdyFrame->GetAndRefreshBodyData(bdyArray);
		for each (Body^ body in bdyArray)
		{
			if (body->IsTracked)
			{
				WFC::IMapView<JointType, Joint>^ joints = body->Joints;
				for each(auto joint in joints)
				{
					if (joint->Key == JointType::HandRight)
					{
						CameraSpacePoint position = joint->Value.Position;
						ColorSpacePoint colorSpacePoint = coordMapper->MapCameraPointToColorSpace(position);
						//body
						rx = colorSpacePoint.X;
						ry = colorSpacePoint.Y;
						

					}
					else if (joint->Key == JointType::HandLeft)
					{
						CameraSpacePoint position = joint->Value.Position;
						ColorSpacePoint colorSpacePoint = coordMapper->MapCameraPointToColorSpace(position);
						lx = colorSpacePoint.X;
						ly = colorSpacePoint.Y;
					}

					else if (joint->Key == JointType::Head)
					{
						CameraSpacePoint position = joint->Value.Position;
						ColorSpacePoint colorSpacePoint = coordMapper->MapCameraPointToColorSpace(position);
						hx = colorSpacePoint.X;
						hy = colorSpacePoint.Y;
					}

					else if (joint->Key == JointType::HandTipRight)
					{
						CameraSpacePoint position = joint->Value.Position;
						ColorSpacePoint colorSpacePoint = coordMapper->MapCameraPointToColorSpace(position);
						htx = colorSpacePoint.X;
						hty = colorSpacePoint.Y;
					}

					else if (joint->Key == JointType::ThumbRight) 
					{
						CameraSpacePoint position = joint->Value.Position;
						ColorSpacePoint colorSpacePoint = coordMapper->MapCameraPointToColorSpace(position);
						thrx = colorSpacePoint.X;
						thry = colorSpacePoint.Y;
					}

					//Karmine fix :-)
					if (body->HandLeftConfidence == TrackingConfidence::High)
					{
						switch (body->HandLeftState)
						{

						case HandState::Open:
								colorLeftHand = ofColor::green;
								break;
						case HandState::Closed:
								colorLeftHand = ofColor::red;
								break;
						case HandState::Lasso:
								colorLeftHand = ofColor::yellow;
								lassoFun = true;
								break;
						default:
								colorLeftHand = ofColor::white;
								lassoFun = false;
						
						}
					}
				}
			}
		}
	}

}
//--------------------------------------------------------------

void ofApp::keyPressed(int key){

	if (key == ' ')
	{
		//switch
		mode++;

		if (mode >= 4)
			mode = 0;

	}

}
//--------------------------------------------------------------

void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

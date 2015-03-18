/*----------------------------------------------------------------------------------------
Author: Bidur Bohara

Project: Volume Rendering in Open Inventor using VolumeViz extension
----------------------------------------------------------------------------------------*/

//header files
#include <Inventor/Xt/SoXt.h>
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>
#include <Inventor/actions/SoBoxHighlightRenderAction.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoColorMap.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoGradientBackground.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoText2.h>
#include <VolumeViz/nodes/SoOrthoSlice.h>
#include <VolumeViz/draggers/SoOrthoSliceDragger.h>
#include <VolumeViz/nodes/SoVolumeData.h>
#include <VolumeViz/nodes/SoVolumeRender.h>
#include <VolumeViz/nodes/SoVolumeRenderingQuality.h>
#include <VolumeViz/readers/SoVRRasterStackReader.h>
#include <LDM/manips/SoROIManip.h>
#include <LDM/nodes/SoDataRange.h>
#include <LDM/nodes/SoTransferFunction.h>
#include <VolumeViz/nodes/SoVolumeRendering.h>

#include <string>
#include <fstream>
#include <iomanip>

#define TeddyImagesPath	"D:/FEI-OpenInv/teddybear/"
#define TeddyLSTFile	"teddybear.lst"		// Please modify the path accordingly

/* Hacked implementation to create a LST file for volume rendering.
 * The LST file should be created once, and used there after.
 */
void CreateLSTFile(const std::string &_path, std::string _name, const int imageCount)
{
	if(_path.empty() || _name.empty())
		return;
	
	_name.append(".lst");
	
	std::ofstream out(_name);
	out << "Parameters {\n\tSize 0.0 0.0 0.0 128.0 128.0 128.0\n}\n";

	for(size_t i = 0; i < imageCount; ++i)
		out << _path << "teddybear" << std::setw(3) << std::setfill('0') << i << ".jpg\n";
	out.close();
}

/* Handles KeyPress event to switch between SoSwitch nodes.
 * Receives first SoSwitch node in the graph as userData
 * Press N: to change SoSwitch whichChile to next node
 * Press P: to change SoSwitch whichChile to previous node
 * Pressing keys N and P behavs as toggle between OrthoSlice and VolumeRender
 * Press R: to toggle between show/hide ROIManip in VolumeRender
 * Press A: to traverse both nodes of SoSwitch. Press R at this state to show ROI
 */
void keyPressCallBack(void* userData, SoEventCallback* eventCB)
{
	SoSwitch* pSwitch = (SoSwitch*)userData;
	
	int nChild = pSwitch->getNumChildren();
	int nIndex = pSwitch->whichChild.getValue();
	
	const SoEvent* event = eventCB->getEvent();
	//if(SO_KEY_PRESS_EVENT(event, LEFT_ARROW))
	if(SO_KEY_PRESS_EVENT(event, N))
	{
		nIndex++;
		nIndex = nIndex % nChild;
	}
	else if(SO_KEY_PRESS_EVENT(event, P))
	{
		nIndex--;
		nIndex = (nIndex % nChild) < 0 ? nChild-1 : nIndex % nChild ;
	}
	else if(SO_KEY_PRESS_EVENT(event, A))
	{
		nIndex = -3;
	}
	else if(SO_KEY_PRESS_EVENT(event, R))
	{
		// There should be an elegant way to do this!!
		if(nIndex == 1 || nIndex == -3) // Volume Rendering
		{
			SoSwitch* vNode = (SoSwitch*)pSwitch->getChild(1);
			SoSwitch *vSwitch = (SoSwitch*)vNode->getChild(vNode->getNumChildren()-1);
			vSwitch->whichChild = vSwitch->whichChild.getValue() == 1 ? -3 : 1;
			vSwitch->whichChild.touch();
		}
	}

	pSwitch->whichChild = nIndex;
	pSwitch->whichChild.touch();
	eventCB->setHandled();
}

/* Creates bounding box of the volume data using SoIndexLineSet and 
 * SoVertexProperty. This function is same as the one in GettingStarted.pdf.
 */
SoSeparator *createVolBBox(SbBox3f &volSize)
{
  // The box will be easier to see without lighting and with wide lines
  SoLightModel *pLModel = new SoLightModel;
  pLModel->model = SoLightModel::BASE_COLOR;

  SoDrawStyle *pStyle = new SoDrawStyle;
  pStyle->lineWidth = 2;

  // Create a cube with the geometric size of the volume
  float xmin, xmax, ymin, ymax, zmin, zmax;
  volSize.getBounds( xmin,ymin, zmin, xmax, ymax, zmax );

  SoVertexProperty *pProp = new SoVertexProperty;
  pProp->vertex.set1Value( 0, SbVec3f(xmin,ymin,zmin) );
  pProp->vertex.set1Value( 1, SbVec3f(xmax,ymin,zmin) );
  pProp->vertex.set1Value( 2, SbVec3f(xmax,ymax,zmin) );
  pProp->vertex.set1Value( 3, SbVec3f(xmin,ymax,zmin) );
  pProp->vertex.set1Value( 4, SbVec3f(xmin,ymin,zmax) );
  pProp->vertex.set1Value( 5, SbVec3f(xmax,ymin,zmax) );
  pProp->vertex.set1Value( 6, SbVec3f(xmax,ymax,zmax) );
  pProp->vertex.set1Value( 7, SbVec3f(xmin,ymax,zmax) );
  pProp->orderedRGBA.set1Value(0, 0x55557FFF);

  // Draw it with a line set
  int coordIndices[] = {0,1,2,3,0,-1,
						4,5,6,7,4,-1,
                        0,4,-1, 
						1,5,-1, 
						2,6,-1, 
						3,7};
  int numCoordIndices = sizeof(coordIndices)/sizeof(int);
  SoIndexedLineSet *pLines = new SoIndexedLineSet;
  pLines->vertexProperty = pProp;
  pLines->coordIndex.setValues( 0, numCoordIndices, coordIndices );

  // Assemble scene graph
  SoSeparator *pBoxSep = new SoSeparator;
  pBoxSep->addChild( pLModel );
  pBoxSep->addChild( pStyle );
  pBoxSep->addChild( pLines );
  return pBoxSep;
}

/* Create SoSeparator node for OrthoSlice and add to it's parent node
 * passed as first argument 'head' to the function.
 */
void createOthroSlice(SoGroup* head, SoVolumeData* volumeData)
{
	SbVec3i32 volDim = volumeData->getDimension();
	SoSeparator *orthoNode = new SoSeparator;

	// Add Ortho node to the head node
	head->addChild(orthoNode);

	// In Teddy bear, the value range is [0, 255]
	// We are resetting the range to change "window center" & "window width"
	SoDataRange *pRange = new SoDataRange();
	pRange->mapOnFullColorRange = false;
	pRange->min = 52;
	pRange->max = 128;
	orthoNode->addChild(pRange);

	// Use a predefined colorMap with the SoTransferFunction 
	SoTransferFunction* pTransFunc = new SoTransferFunction;
	pTransFunc->predefColorMap = SoTransferFunction::INTENSITY;
	orthoNode->addChild(pTransFunc);

	// Define three orthoslices for each axis-aligned plane. Set alpha value of 
	// slices to opaque and set the starting position to mid-value in the dimension 
	SoOrthoSlice* pSliceX = new SoOrthoSlice;
	pSliceX->axis = SoOrthoSlice::X;
	pSliceX->sliceNumber = volDim[0]/2;
	pSliceX->alphaUse = SoOrthoSlice::ALPHA_OPAQUE;
	orthoNode->addChild(pSliceX);

	SoOrthoSlice* pSliceY = new SoOrthoSlice;
	pSliceY->axis = SoOrthoSlice::Y;
	pSliceY->sliceNumber = volDim[1]/2;
	pSliceX->alphaUse = SoOrthoSlice::ALPHA_OPAQUE;
	orthoNode->addChild(pSliceY);

	SoOrthoSlice* pSliceZ = new SoOrthoSlice;
	pSliceZ->axis = SoOrthoSlice::Z;
	pSliceZ->sliceNumber = volDim[2]/2;
	pSliceX->alphaUse = SoOrthoSlice::ALPHA_OPAQUE;
	orthoNode->addChild(pSliceZ);
	
	// Create path to slice nodes
	SoPath *pPathX = new SoPath(orthoNode);
	SoPath *pPathY = new SoPath(orthoNode);
	SoPath *pPathZ = new SoPath(orthoNode);

	// Create manipulator or dragger for all three orthoslices
	SoOrthoSliceDragger *pDraggerX = new SoOrthoSliceDragger();
	pDraggerX->volumeDimension = volumeData->data.getSize();
	pDraggerX->volumeExtent    = volumeData->extent.getValue();
	orthoNode->addChild( pDraggerX );

	SoOrthoSliceDragger *pDraggerY = new SoOrthoSliceDragger();
	pDraggerY->orthoSlicePath  = pPathY;
	pDraggerY->volumeDimension = volumeData->data.getSize();
	pDraggerY->volumeExtent    = volumeData->extent.getValue();
	orthoNode->addChild( pDraggerY );

	SoOrthoSliceDragger *pDraggerZ = new SoOrthoSliceDragger();
	pDraggerZ->volumeDimension = volumeData->data.getSize();
	pDraggerZ->volumeExtent    = volumeData->extent.getValue();
	orthoNode->addChild( pDraggerZ );

	pPathX->setHead(pSliceX);
	pPathY->setHead(pSliceY);
	pPathZ->setHead(pSliceZ);

	// Set path of the Ortho slice draggers to corresponding Orthoslices
	pDraggerX->orthoSlicePath  = pPathX;
	pDraggerY->orthoSlicePath  = pPathY;
	pDraggerZ->orthoSlicePath  = pPathZ;
}

/* Create SoSeparator node for VolumeRendering and add to it's parent node
 * passed as first argument 'head' to the function.
 */
void createVolumeRender(SoGroup* head, SoVolumeData* volumeData)
{
	SbVec3i32 volDim = volumeData->getDimension();
	SoSeparator *volRenderNode = new SoSeparator;

	// Add Render node to the head node
	head->addChild(volRenderNode);

	// Set material properties for volume rendering
	SoMaterial* volMaterial = new SoMaterial();
	volMaterial->diffuseColor.setValue(1.f, 1.f, 1.f);
	volMaterial->ambientColor.setValue(.2f, .2f, .2f);
	volMaterial->specularColor.setValue(.2f, .2f, .2f);
	volMaterial->shininess.setValue(.0f);
	volRenderNode->addChild(volMaterial);

	// Set Render Quality properties
	SoVolumeRenderingQuality* volQual = new SoVolumeRenderingQuality();
	volQual->deferredLighting = TRUE;	// Lighting computed at GPU
	volQual->preIntegrated = TRUE;
	volQual->surfaceScalarExponent = 2;	// Removes noise due to small gradients
	volRenderNode->addChild(volQual);

	// Set min and max value for Transfer function by fixed value
	// Better implementation would be to set them via DialogViz
	SoTransferFunction* vTF = new SoTransferFunction;
	vTF->predefColorMap = SoTransferFunction::INTENSITY;
	vTF->minValue = 52;
	vTF->maxValue = 128;
	volRenderNode->addChild(vTF);

	SoSwitch* volSwitch = new SoSwitch();
	volSwitch->whichChild = 1;
	volRenderNode->addChild(volSwitch);

	// Define ROI manipulator to set and manipulate region of interest
	// Set it to constraint within volume. Initial box-dim is set to volume dim
	SoROIManip* pRoiManip = new SoROIManip();
	pRoiManip->constrained = TRUE;
	pRoiManip->box.setValue(SbVec3i32(volDim[0]/4, volDim[1]/4, volDim[2]/4), 
							SbVec3i32(volDim[0]*.8, volDim[1]*.8, volDim[2]*.8));
	volSwitch->addChild(pRoiManip);

	// Node in charge of drawing the volume
	SoVolumeRender* volRender = new SoVolumeRender;
	volRender->lowResMode = SoVolumeRender::DECREASE_SCREEN_RESOLUTION;
	volRender->lowScreenResolutionScale = 2;
	volRender->samplingAlignment = SoVolumeRender::SMOOTH_BOUNDARY_ALIGNED;
	volRender->numSlicesControl = SoVolumeRender::ALL;
	volRender->interpolation = SoVolumeRender::CUBIC;
	volRender->subdivideTile = TRUE;	// LDM tiles will be subdivided
	volRender->opacityThreshold = .1f;	// Voxels with alpha value > 0.1 are taken as solid/opaque
	volSwitch->addChild(volRender);
	
}

/* main function
 * Creates custom volume reader for stacked raster images
 * Creates OIV scene graph, and ExaminerViewer
 */
int main(int argc, char **argv)
{
	// Check argument to the program, and assign first argument as
	// lst filename. In case of no arguments assign default name.
	SbString lstFile = TeddyLSTFile;
	if(argc > 1)
		lstFile = argv[1];

	// Initialize OpenInventor and create the window
	Widget mainWindow = SoXt::init(argv[0]);
	if (!mainWindow) return 0;

#if 0
	// Create LST file from set of input images for volume rendering
	CreateLSTFile(TeddyImagesPath, "teddybear", 62);
#endif

	// Initialize of VolumeViz extension
	SoVolumeRendering::init();

	// Cumstom VolumeReader for stacked raster images
	SoVRRasterStackReader* stackReader = new SoVRRasterStackReader();
	// If reader cannot fine file, then just return for now.
	if(stackReader->setFilename(lstFile) != 0)
	{
		std::cerr << "Unable to open .LST file. @ line " << __LINE__ 
				  << " in " << __FUNCTION__ << "() function.\n" ;
		SoVolumeRendering::finish();
		SoXt::finish();
		return 0;
	}

	/***************************************************************************/
	/*			           Begin of Scene Graph                                */
	/***************************************************************************/
	// Top most root node
	SoSeparator *myRoot = new SoSeparator;
	myRoot->ref();

	// Get Size and Dimension information about the input volume data
	// Used to set the starting positions of ortho-slices
	SbBox3f volSize;
	SbVec3i32 volDim;
	SoVolumeData::DataType dType;
	stackReader->getDataChar(volSize, dType, volDim);

	// Uses a default gradient background colors
	SoGradientBackground *viewerBackground = new SoGradientBackground;
	myRoot->addChild(viewerBackground);

	// Rotate the dataset by 90 degree along x-axis to make Teddy face the camera
	SoTransform* pTransform = new SoTransform;
	pTransform->rotation = SbRotation(SbVec3f(1.f, .0f, .0f), (float)M_PI_2);
	myRoot->addChild(pTransform);

	// Create bounding box of the volume data and add to the root node
	myRoot->addChild(createVolBBox(volSize));
	
	// Node to hold the volume data. Volume data is shared by render nodes
	// Specify the reader object for volume data
	SoVolumeData* pVolData = new SoVolumeData();
	pVolData->setReader(*stackReader);
	myRoot->addChild( pVolData );

	// Switch node to switch between Orthoslice rendering (node 0) and
	// VolumeRender (node 1). Orthoslice is selected by default.
	// Press either key 'N' or 'P' to switch to next or previous node. 
	SoSwitch* mySwitch = new SoSwitch();
	mySwitch->whichChild = 0;
	myRoot->addChild(mySwitch);

	// Register EventCallback to handle KeyPressEvent
	SoEventCallback *myEventCB = new SoEventCallback();
	myEventCB->addEventCallback(SoKeyboardEvent::getClassTypeId(), keyPressCallBack, (void*)mySwitch);
	myRoot->addChild(myEventCB);

	// Create a Node for Orthoslice rendering
	// Pass a pointer to parent node and SoVolumeData
	createOthroSlice(mySwitch, pVolData);

	// Create a node for volume rendering; Pass a pointer to pointer node 
	// as SoVolumeData
	createVolumeRender(mySwitch, pVolData);

	/***************************************************************************/
	/*			             End of Scene Graph                                */
	/***************************************************************************/
	
	// Set up the viewer
	SoXtExaminerViewer *mainViewer = new SoXtExaminerViewer(mainWindow);
	mainViewer->setSceneGraph(myRoot);
	mainViewer->setTitle("Teddy Bear Rendering");
	mainViewer->show();

	// Closing and cleaning up
	SoXt::show(mainWindow);
	SoXt::mainLoop();
	delete mainViewer;
	myRoot->unref();
	SoVolumeRendering::finish();
	SoXt::finish();

	return 0;
}




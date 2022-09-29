//
// camera.h
//

#define	CAMERA_DEFAULT_DIST_FROM_ME	700.0f
#define	DEFAULT_CAMERA_YOFF			100.0f

void InitCamera(void);
void UpdateCamera(void);
	void DrawLensFlare(void);




void PrepAnaglyphCameras(void);
void RestoreCamerasFromAnaglyph(void);
void CalcAnaglyphCameraOffset(Byte pass);

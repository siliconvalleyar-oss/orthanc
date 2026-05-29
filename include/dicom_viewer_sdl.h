#ifndef DICOM_VIEWER_SDL_H
#define DICOM_VIEWER_SDL_H

#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include "orthanc_client.h"
#include "json.hpp"

using json = nlohmann::json;

struct DcmtkImageDeleter {
    void operator()(void* p) const;
};

// ============================================================
// Annotation types
// ============================================================

// ============================================================
// Image processing filter types
// ============================================================
enum class ImageFilter {
    NONE,
    HISTOGRAM_EQUALIZE,
    SMOOTH,
    EDGE_DETECT
};

enum class AnnotationMode {
    WL,          // Window/Level (default)
    DISTANCE,    // Distance measurement
    ANGLE,       // Angle measurement
    ROI,         // Region of Interest
    TEXT         // Free text label
};

struct ImagePoint {
    double x, y;
    ImagePoint() : x(0), y(0) {}
    ImagePoint(double x_, double y_) : x(x_), y(y_) {}
};

struct DistanceAnnotation {
    ImagePoint p1, p2;
    double pixelSpacing;
    double distanceMM;
    bool complete;
    DistanceAnnotation() : pixelSpacing(1.0), distanceMM(0), complete(false) {}
};

struct AngleAnnotation {
    ImagePoint vertex, arm1, arm2;
    double angleDeg;
    int pointsPlaced;
    AngleAnnotation() : angleDeg(0), pointsPlaced(0) {}
};

struct ROIAnnotation {
    ImagePoint corner1, corner2;
    bool complete;
    double minVal, maxVal, meanVal;
    ROIAnnotation() : complete(false), minVal(0), maxVal(0), meanVal(0) {}
};

struct TextAnnotation {
    ImagePoint pos;
    std::string text;
    unsigned char r, g, b;
    TextAnnotation() : r(255), g(255), b(255) {}
    TextAnnotation(const ImagePoint& p, const std::string& t, unsigned char cr=255, unsigned char cg=255, unsigned char cb=100)
        : pos(p), text(t), r(cr), g(cg), b(cb) {}
};

// ============================================================
// Annotation snapshot for Undo/Redo
// ============================================================
struct AnnotationSnapshot {
    std::vector<DistanceAnnotation> distances;
    std::vector<AngleAnnotation> angles;
    std::vector<ROIAnnotation> rois;
    std::vector<TextAnnotation> texts;
};

// ============================================================
// DicomViewerSDL
// ============================================================

class DicomViewerSDL {
public:
    DicomViewerSDL(OrthancClient& client,
                   const std::string& instanceId,
                   const std::string& windowTitle = "Orthanc DICOM Viewer");

    // Constructor for series navigation
    DicomViewerSDL(OrthancClient& client,
                   const std::vector<std::string>& seriesInstanceIds,
                   int startIndex = 0,
                   const std::string& windowTitle = "Orthanc DICOM Viewer");

    ~DicomViewerSDL();

    int Run();

private:
    OrthancClient& client_;
    std::vector<std::string> instanceIds_;
    int currentSeriesIndex_;
    bool seriesMode_;
    std::string windowTitle_;

    // Window/Level state
    double windowCenter_;
    double windowWidth_;
    double zoomFactor_;
    double panX_;
    double panY_;
    bool autoWindowLevel_;
    bool imageLoaded_;

    // Image dimensions
    unsigned long dicomWidth_;
    unsigned long dicomHeight_;
    int dicomDepth_;

    // Pixel data
    std::vector<unsigned char> pixelData_;
    std::vector<unsigned char> basePixelData_;   // WL-rendered original (pre-filter)
    std::vector<unsigned char> rgbaBuffer_;

    // Image filter state
    ImageFilter currentFilter_;

    // DICOM dataset in memory
    std::unique_ptr<void, DcmtkImageDeleter> dicomDataset_;
    std::string dicomTempPath_;

    // Pixel spacing from DICOM (0028,0030)
    double pixelSpacingX_;
    double pixelSpacingY_;

    // Mouse state
    bool mouseDraggingWL_;
    bool mouseDraggingPan_;
    bool mouseDraggingROI_;
    int lastMouseX_;
    int lastMouseY_;

    // Annotation state
    AnnotationMode currentMode_;
    std::vector<DistanceAnnotation> distances_;
    std::vector<AngleAnnotation> angles_;
    std::vector<ROIAnnotation> rois_;
    std::vector<TextAnnotation> texts_;

    // In-progress annotation
    DistanceAnnotation pendingDistance_;
    AngleAnnotation pendingAngle_;
    ROIAnnotation pendingROI_;
    TextAnnotation pendingText_;
    bool placingAnnotation_;
    bool textInputActive_;

    // Undo/Redo
    std::vector<AnnotationSnapshot> undoStack_;
    std::vector<AnnotationSnapshot> redoStack_;

    // SDL handles
    void* sdlWindow_;
    void* sdlRenderer_;
    void* sdlTexture_;

    // Private methods
    bool LoadInstance(int index);
    bool DownloadFromOrthanc();
    bool LoadDicomFromOrthanc();
    bool InitSDL(int w, int h);
    void RenderFrame();
    void UpdateWindowTitle();
    void HandleMouseMotion(int x, int y);
    void HandleMouseButton(int button, bool pressed, int x, int y);
    void HandleMouseWheel(int delta);
    void HandleTextInput(const char* text);
    void HandleKey(int key, unsigned short mod);
    void ResetView();
    void RenderWithWindowLevel(double center, double width);
    void CleanupSDL();

    // Annotation methods
    void SetMode(AnnotationMode mode);
    ImagePoint ScreenToImage(int sx, int sy);
    void PlaceAnnotationPoint(int sx, int sy);
    void FinishTextAnnotation();
    void CancelPendingAnnotation();
    void PushUndoState();
    void Undo();
    void Redo();
    void RenderAnnotations();
    void RenderDistanceOverlay(const DistanceAnnotation& da);
    void RenderAngleOverlay(const AngleAnnotation& aa);
    void RenderROIOverlay(const ROIAnnotation& ra);
    void RenderTextOverlay(const TextAnnotation& ta);
    void ComputeROIStats(ROIAnnotation& ra);
    double GetPixelValue(unsigned long ix, unsigned long iy) const;

    // Image filter methods
    void ApplyFilter(ImageFilter filter);
    void ApplyHistogramEqualize();
    void ApplySmooth();
    void ApplyEdgeDetect();

    // SR Export
    bool ExportSR(const std::string& outputPath);

    // Label drawing
    void DrawString(int sx, int sy, const char* text,
                    unsigned char r, unsigned char g, unsigned char b);
    void DrawChar(int sx, int sy, char c,
                  unsigned char r, unsigned char g, unsigned char b);
};

#endif // DICOM_VIEWER_SDL_H

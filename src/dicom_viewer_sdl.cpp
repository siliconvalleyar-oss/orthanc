/**
 * @brief Visor DICOM SDL2 completo
 *
 * - Window/Level (click izq + arrastrar)
 * - Zoom (rueda), Pan (click der + arrastrar)
 * - Distancia (D): dos clics, muestra mm
 * - Angulo (A): tres clics, muestra grados
 * - ROI (O): click-arrastrar-soltar, min/max/mean
 * - Texto libre (T): clic + tipear, Enter finaliza
 * - Undo/Redo (Ctrl+Z / Ctrl+Y)
 * - Navegacion de serie (Flechas Izq/Der)
 * - Exportar SR (Ctrl+E)
 */

#include "dicom_viewer_sdl.h"

// DCMTK
#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmdata/dcfilefo.h>
#include <dcmtk/dcmdata/dcdatset.h>
#include <dcmtk/dcmdata/dcdeftag.h>
#include <dcmtk/dcmdata/dcxfer.h>
#include <dcmtk/dcmdata/dcvrobow.h>
#include <dcmtk/dcmdata/dcsequen.h>
#include <dcmtk/dcmdata/dcvrlo.h>
#include <dcmtk/dcmdata/dcvrpn.h>
#include <dcmtk/dcmdata/dcvrsh.h>
#include <dcmtk/dcmdata/dcvrui.h>
#include <dcmtk/dcmdata/dcvrcs.h>
#include <dcmtk/dcmdata/dcvrda.h>
#include <dcmtk/dcmdata/dcvrtm.h>
#include <dcmtk/dcmdata/dcuid.h>

// SDL2
#include <SDL2/SDL.h>

// libcurl
#include <curl/curl.h>

// Standard
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <ctime>

// ============================================================
// DcmtkImageDeleter
// ============================================================
void DcmtkImageDeleter::operator()(void* p) const {
    if (p) delete static_cast<DcmFileFormat*>(p);
}

// ============================================================
// libcurl callback
// ============================================================
static size_t WriteToVecCb(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* v = static_cast<std::vector<unsigned char>*>(userp);
    v->insert(v->end(), static_cast<unsigned char*>(contents),
              static_cast<unsigned char*>(contents) + total);
    return total;
}

// ============================================================
// 5x7 bitmap font data (ASCII 32-127)
// ============================================================
static const unsigned char font5x7[96][5] = {
    {0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x00,0x07,0x00,0x07,0x00},
    {0x14,0x7F,0x14,0x7F,0x14},{0x24,0x2A,0x7F,0x2A,0x12},{0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50},{0x00,0x05,0x03,0x00,0x00},{0x00,0x1C,0x22,0x41,0x00},
    {0x00,0x41,0x22,0x1C,0x00},{0x14,0x08,0x3E,0x08,0x14},{0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00},{0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x02},{0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},{0x18,0x14,0x12,0x7F,0x10},
    {0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x36,0x36,0x00,0x00},
    {0x00,0x56,0x36,0x00,0x00},{0x08,0x14,0x22,0x41,0x00},{0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08},{0x02,0x01,0x51,0x09,0x06},{0x32,0x49,0x79,0x41,0x3E},
    {0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
    {0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},{0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20},{0x00,0x41,0x41,0x7F,0x00},{0x04,0x02,0x01,0x02,0x04},
    {0x40,0x40,0x40,0x40,0x40},{0x00,0x01,0x02,0x04,0x00},{0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38},{0x38,0x44,0x44,0x44,0x20},{0x38,0x44,0x44,0x48,0x7F},
    {0x38,0x54,0x54,0x54,0x18},{0x08,0x7E,0x09,0x01,0x02},{0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78},{0x00,0x44,0x7D,0x40,0x00},{0x20,0x40,0x44,0x3D,0x00},
    {0x7F,0x10,0x28,0x44,0x00},{0x00,0x41,0x7F,0x40,0x00},{0x7C,0x04,0x18,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78},{0x38,0x44,0x44,0x44,0x38},{0x7C,0x14,0x14,0x14,0x08},
    {0x08,0x14,0x14,0x18,0x7C},{0x7C,0x08,0x04,0x04,0x08},{0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20},{0x3C,0x40,0x40,0x20,0x7C},{0x1C,0x20,0x40,0x20,0x1C},
    {0x3C,0x40,0x30,0x40,0x3C},{0x44,0x28,0x10,0x28,0x44},{0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44}
};

// ============================================================
// Constructor / Destructor
// ============================================================
DicomViewerSDL::DicomViewerSDL(OrthancClient& client,
                               const std::string& instanceId,
                               const std::string& windowTitle)
    : client_(client)
    , currentSeriesIndex_(0), seriesMode_(false)
    , windowTitle_(windowTitle)
    , windowCenter_(0), windowWidth_(0), zoomFactor_(1.0)
    , panX_(0), panY_(0), autoWindowLevel_(true), imageLoaded_(false)
    , dicomWidth_(0), dicomHeight_(0), dicomDepth_(0)
    , currentFilter_(ImageFilter::NONE)
    , pixelSpacingX_(1.0), pixelSpacingY_(1.0)
    , mouseDraggingWL_(false), mouseDraggingPan_(false), mouseDraggingROI_(false)
    , lastMouseX_(0), lastMouseY_(0)
    , currentMode_(AnnotationMode::WL)
    , placingAnnotation_(false), textInputActive_(false)
    , sdlWindow_(nullptr), sdlRenderer_(nullptr), sdlTexture_(nullptr)
{
    instanceIds_.push_back(instanceId);
}

DicomViewerSDL::DicomViewerSDL(OrthancClient& client,
                               const std::vector<std::string>& seriesInstanceIds,
                               int startIndex,
                               const std::string& windowTitle)
    : client_(client), instanceIds_(seriesInstanceIds)
    , currentSeriesIndex_(startIndex), seriesMode_(true)
    , windowTitle_(windowTitle)
    , windowCenter_(0), windowWidth_(0), zoomFactor_(1.0)
    , panX_(0), panY_(0), autoWindowLevel_(true), imageLoaded_(false)
    , dicomWidth_(0), dicomHeight_(0), dicomDepth_(0)
    , currentFilter_(ImageFilter::NONE)
    , pixelSpacingX_(1.0), pixelSpacingY_(1.0)
    , mouseDraggingWL_(false), mouseDraggingPan_(false), mouseDraggingROI_(false)
    , lastMouseX_(0), lastMouseY_(0)
    , currentMode_(AnnotationMode::WL)
    , placingAnnotation_(false), textInputActive_(false)
    , sdlWindow_(nullptr), sdlRenderer_(nullptr), sdlTexture_(nullptr)
{}

DicomViewerSDL::~DicomViewerSDL() {
    CleanupSDL();
    if (!dicomTempPath_.empty()) std::remove(dicomTempPath_.c_str());
}

// ============================================================
// Load instance by index (for series navigation)
// ============================================================
bool DicomViewerSDL::LoadInstance(int index) {
    if (index < 0 || index >= (int)instanceIds_.size()) return false;
    currentSeriesIndex_ = index;

    // Reset state
    distances_.clear();
    angles_.clear();
    rois_.clear();
    texts_.clear();
    undoStack_.clear();
    redoStack_.clear();
    placingAnnotation_ = false;
    textInputActive_ = false;
    zoomFactor_ = 1.0f;
    panX_ = 0.0; panY_ = 0.0;
    windowCenter_ = 0; windowWidth_ = 0;
    autoWindowLevel_ = true;
    currentFilter_ = ImageFilter::NONE;
    basePixelData_.clear();
    imageLoaded_ = false;

    if (!dicomTempPath_.empty()) {
        std::remove(dicomTempPath_.c_str());
        dicomTempPath_.clear();
    }
    dicomDataset_.reset();

    return LoadDicomFromOrthanc();
}

// ============================================================
// Download from Orthanc via libcurl
// ============================================================
bool DicomViewerSDL::DownloadFromOrthanc() {
    dicomTempPath_ = "/tmp/orthanc_viewer_" + instanceIds_[currentSeriesIndex_] + ".dcm";
    std::cout << "  Descargando instancia DICOM..." << std::endl;

    CURL* curl = curl_easy_init();
    if (!curl) { std::cerr << "  [ERROR] curl init\n"; return false; }

    std::string url = client_.GetBaseUrl() + "/instances/" + instanceIds_[currentSeriesIndex_] + "/file";
    std::vector<unsigned char> data;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToVecCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    long http = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK || http >= 400 || data.empty()) {
        std::cerr << "  [ERROR] HTTP " << http << std::endl; return false;
    }

    std::ofstream of(dicomTempPath_, std::ios::binary);
    if (!of.is_open()) { std::cerr << "  [ERROR] temp file\n"; return false; }
    of.write((const char*)data.data(), data.size()); of.close();
    std::cout << "  " << data.size() << " bytes.\n";
    return true;
}

// ============================================================
// Render with window/level from in-memory dataset
// ============================================================
void DicomViewerSDL::RenderWithWindowLevel(double center, double width) {
    if (!dicomDataset_) return;
    auto* ff = static_cast<DcmFileFormat*>(dicomDataset_.get());
    DicomImage* di = new DicomImage(ff->getDataset(), EXS_Unknown, 0UL, 0UL, 0UL);
    if (!di || di->getStatus() != EIS_Normal) { delete di; return; }
    if (width > 0) di->setWindow(center, width);
    else di->setMinMaxWindow(1);

    const void* raw = di->getOutputData(8, 0);
    if (raw) {
        unsigned long sz = di->getOutputDataSize(8);
        pixelData_.resize(sz);
        basePixelData_.resize(sz);
        if (sz > 0) {
            std::memcpy(pixelData_.data(), raw, sz);
            std::memcpy(basePixelData_.data(), raw, sz);
        }
        // Re-apply active filter
        if (currentFilter_ != ImageFilter::NONE) {
            ApplyFilter(currentFilter_);
        }
    }
    delete di;
}

// ============================================================
// Load DICOM
// ============================================================
bool DicomViewerSDL::LoadDicomFromOrthanc() {
    if (!DownloadFromOrthanc()) return false;
    std::cout << "  Procesando con DCMTK..." << std::endl;

    DcmFileFormat* ff = new DcmFileFormat();
    OFCondition st = ff->loadFile(dicomTempPath_.c_str());
    if (!st.good()) {
        std::cerr << "  [ERROR] " << st.text() << std::endl;
        delete ff; return false;
    }
    dicomDataset_.reset(ff);
    DcmDataset* ds = ff->getDataset();

    // Read pixel spacing
    const Float64* pixelVals = nullptr;
    unsigned long vm = 0;
    if (ds->findAndGetFloat64Array(DCM_PixelSpacing, pixelVals, &vm).good() && vm >= 2) {
        pixelSpacingX_ = pixelVals[0];
        pixelSpacingY_ = pixelVals[1];
        std::cout << "  PixelSpacing: " << pixelSpacingX_ << " x " << pixelSpacingY_ << " mm\n";
    } else {
        pixelSpacingX_ = pixelSpacingY_ = 1.0;
        std::cout << "  PixelSpacing no encontrado, usando 1.0 mm/pixel\n";
    }

    DicomImage* di = new DicomImage(ds, EXS_Unknown, 0UL, 0UL, 0UL);
    if (!di || di->getStatus() != EIS_Normal) {
        std::cerr << "  [ERROR] imagen invalida\n"; delete di; return false;
    }
    dicomWidth_ = di->getWidth();
    dicomHeight_ = di->getHeight();
    dicomDepth_ = di->getDepth();
    std::cout << "  " << dicomWidth_ << "x" << dicomHeight_
              << " x" << dicomDepth_ << " bits, "
              << di->getFrameCount() << " frame(s)\n";
    delete di;

    RenderWithWindowLevel(0, 0);
    imageLoaded_ = true;
    std::cout << "  \033[32mListo\033[0m\n";
    return true;
}

// ============================================================
// SDL Init
// ============================================================
bool DicomViewerSDL::InitSDL(int w, int h) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) return false;
    sdlWindow_ = SDL_CreateWindow(windowTitle_.c_str(), SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!sdlWindow_) { SDL_Quit(); return false; }
    sdlRenderer_ = SDL_CreateRenderer((SDL_Window*)sdlWindow_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdlRenderer_) { SDL_DestroyWindow((SDL_Window*)sdlWindow_); SDL_Quit(); return false; }
    sdlTexture_ = SDL_CreateTexture((SDL_Renderer*)sdlRenderer_,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, dicomWidth_, dicomHeight_);
    if (!sdlTexture_) { SDL_DestroyRenderer((SDL_Renderer*)sdlRenderer_);
        SDL_DestroyWindow((SDL_Window*)sdlWindow_); SDL_Quit(); return false; }
    rgbaBuffer_.resize(dicomWidth_ * dicomHeight_ * 4);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    return true;
}

// ============================================================
// Expand grayscale 8-bit -> RGBA
// ============================================================
static void ExpandToRGBA(const unsigned char* gray, unsigned long w, unsigned long h,
                          std::vector<unsigned char>& rgba) {
    size_t n = (size_t)w * h;
    rgba.resize(n * 4);
    for (size_t i = 0; i < n; i++) {
        unsigned char v = gray[i];
        rgba[i*4+0] = v; rgba[i*4+1] = v;
        rgba[i*4+2] = v; rgba[i*4+3] = 255;
    }
}

// ============================================================
// Coordinate conversion: screen <-> image
// ============================================================
ImagePoint DicomViewerSDL::ScreenToImage(int sx, int sy) {
    if (!sdlWindow_) return ImagePoint(0,0);
    int winW, winH;
    SDL_GetWindowSize((SDL_Window*)sdlWindow_, &winW, &winH);
    double ix = (sx - (winW - dicomWidth_ * zoomFactor_) * 0.5 - panX_) / zoomFactor_;
    double iy = (sy - (winH - dicomHeight_ * zoomFactor_) * 0.5 - panY_) / zoomFactor_;
    return ImagePoint(ix, iy);
}

// ============================================================
// Get pixel value
// ============================================================
double DicomViewerSDL::GetPixelValue(unsigned long ix, unsigned long iy) const {
    if (ix >= dicomWidth_ || iy >= dicomHeight_ || pixelData_.empty()) return 0;
    return pixelData_[iy * dicomWidth_ + ix];
}

// ============================================================
// Compute ROI statistics
// ============================================================
void DicomViewerSDL::ComputeROIStats(ROIAnnotation& ra) {
    unsigned long x1 = (unsigned long)std::min(ra.corner1.x, ra.corner2.x);
    unsigned long y1 = (unsigned long)std::min(ra.corner1.y, ra.corner2.y);
    unsigned long x2 = (unsigned long)std::max(ra.corner1.x, ra.corner2.x);
    unsigned long y2 = (unsigned long)std::max(ra.corner1.y, ra.corner2.y);
    if (x1 >= dicomWidth_) x1 = dicomWidth_ - 1;
    if (y1 >= dicomHeight_) y1 = dicomHeight_ - 1;
    if (x2 >= dicomWidth_) x2 = dicomWidth_ - 1;
    if (y2 >= dicomHeight_) y2 = dicomHeight_ - 1;

    double sum = 0, minV = 1e9, maxV = -1e9;
    unsigned long count = 0;
    for (unsigned long iy = y1; iy <= y2; iy++) {
        for (unsigned long ix = x1; ix <= x2; ix++) {
            double v = pixelData_[iy * dicomWidth_ + ix];
            sum += v;
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;
            count++;
        }
    }
    ra.minVal = (count > 0) ? minV : 0;
    ra.maxVal = (count > 0) ? maxV : 0;
    ra.meanVal = (count > 0) ? sum / count : 0;
}

// ============================================================
// Draw a single character at screen position using 5x7 font
// ============================================================
void DicomViewerSDL::DrawChar(int sx, int sy, char c,
                               unsigned char r, unsigned char g, unsigned char b) {
    auto* renderer = (SDL_Renderer*)sdlRenderer_;
    if (!renderer || c < 32 || (unsigned char)c > 127) return;
    const unsigned char* glyph = font5x7[c - 32];
    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (glyph[col] & (1 << row)) {
                SDL_RenderDrawPoint(renderer, sx + col, sy + row);
            }
        }
    }
}

void DicomViewerSDL::DrawString(int sx, int sy, const char* text,
                                 unsigned char r, unsigned char g, unsigned char b) {
    int x = sx;
    while (*text) {
        DrawChar(x, sy, *text, r, g, b);
        x += 6;
        text++;
    }
}

// ============================================================
// Undo / Redo
// ============================================================
void DicomViewerSDL::PushUndoState() {
    AnnotationSnapshot snap;
    snap.distances = distances_;
    snap.angles = angles_;
    snap.rois = rois_;
    snap.texts = texts_;
    undoStack_.push_back(snap);
    redoStack_.clear();
    if (undoStack_.size() > 50) {
        undoStack_.erase(undoStack_.begin());
    }
}

void DicomViewerSDL::Undo() {
    if (undoStack_.empty()) return;
    AnnotationSnapshot snap;
    snap.distances = distances_;
    snap.angles = angles_;
    snap.rois = rois_;
    snap.texts = texts_;
    redoStack_.push_back(snap);

    distances_ = undoStack_.back().distances;
    angles_ = undoStack_.back().angles;
    rois_ = undoStack_.back().rois;
    texts_ = undoStack_.back().texts;
    undoStack_.pop_back();
}

void DicomViewerSDL::Redo() {
    if (redoStack_.empty()) return;
    AnnotationSnapshot snap;
    snap.distances = distances_;
    snap.angles = angles_;
    snap.rois = rois_;
    snap.texts = texts_;
    undoStack_.push_back(snap);

    distances_ = redoStack_.back().distances;
    angles_ = redoStack_.back().angles;
    rois_ = redoStack_.back().rois;
    texts_ = redoStack_.back().texts;
    redoStack_.pop_back();
}

// ============================================================
// Render annotation overlays
// ============================================================
void DicomViewerSDL::RenderDistanceOverlay(const DistanceAnnotation& da) {
    auto* renderer = (SDL_Renderer*)sdlRenderer_;
    if (!renderer) return;
    int winW, winH;
    SDL_GetWindowSize((SDL_Window*)sdlWindow_, &winW, &winH);
    double ox = (winW - dicomWidth_ * zoomFactor_) * 0.5 + panX_;
    double oy = (winH - dicomHeight_ * zoomFactor_) * 0.5 + panY_;

    int sx1 = (int)(da.p1.x * zoomFactor_ + ox);
    int sy1 = (int)(da.p1.y * zoomFactor_ + oy);
    int sx2 = (int)(da.p2.x * zoomFactor_ + ox);
    int sy2 = (int)(da.p2.y * zoomFactor_ + oy);

    SDL_SetRenderDrawColor(renderer, 255, 50, 50, 255);
    SDL_RenderDrawLine(renderer, sx1, sy1, sx2, sy2);

    SDL_Rect c1 = {sx1-3, sy1-3, 6, 6};
    SDL_Rect c2 = {sx2-3, sy2-3, 6, 6};
    SDL_RenderDrawRect(renderer, &c1);
    SDL_RenderDrawRect(renderer, &c2);

    if (da.complete) {
        char buf[64];
        if (da.pixelSpacing > 1.01 || da.pixelSpacing < 0.99) {
            std::snprintf(buf, sizeof(buf), "%.1f mm", da.distanceMM);
        } else {
            double distPx = std::sqrt(std::pow(da.p2.x - da.p1.x, 2) +
                                       std::pow(da.p2.y - da.p1.y, 2));
            std::snprintf(buf, sizeof(buf), "%.0f px", distPx);
        }
        int mx = (sx1 + sx2) / 2, my = (sy1 + sy2) / 2;
        int tw = (int)std::strlen(buf) * 6 + 4;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_Rect bg = {mx - tw/2 - 2, my - 10, tw, 14};
        SDL_RenderFillRect(renderer, &bg);
        DrawString(mx - tw/2 + 2, my - 8, buf, 255, 200, 50);
    }
}

void DicomViewerSDL::RenderAngleOverlay(const AngleAnnotation& aa) {
    auto* renderer = (SDL_Renderer*)sdlRenderer_;
    if (!renderer || aa.pointsPlaced < 2) return;
    int winW, winH;
    SDL_GetWindowSize((SDL_Window*)sdlWindow_, &winW, &winH);
    double ox = (winW - dicomWidth_ * zoomFactor_) * 0.5 + panX_;
    double oy = (winH - dicomHeight_ * zoomFactor_) * 0.5 + panY_;

    int vx = (int)(aa.vertex.x * zoomFactor_ + ox);
    int vy = (int)(aa.vertex.y * zoomFactor_ + oy);

    int a1x = (int)(aa.arm1.x * zoomFactor_ + ox);
    int a1y = (int)(aa.arm1.y * zoomFactor_ + oy);

    SDL_SetRenderDrawColor(renderer, 50, 200, 255, 255);
    SDL_RenderDrawLine(renderer, vx, vy, a1x, a1y);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 50, 200, 255, 180);
    SDL_Rect cv = {vx-4, vy-4, 8, 8};
    SDL_RenderFillRect(renderer, &cv);

    if (aa.pointsPlaced >= 3) {
        int a2x = (int)(aa.arm2.x * zoomFactor_ + ox);
        int a2y = (int)(aa.arm2.y * zoomFactor_ + oy);
        SDL_RenderDrawLine(renderer, vx, vy, a2x, a2y);

        double ang1 = std::atan2(aa.arm1.y - aa.vertex.y, aa.arm1.x - aa.vertex.x);
        double ang2 = std::atan2(aa.arm2.y - aa.vertex.y, aa.arm2.x - aa.vertex.x);
        double r = std::min(40.0, std::min(
            std::sqrt(std::pow(a1x-vx,2)+std::pow(a1y-vy,2)),
            std::sqrt(std::pow(a2x-vx,2)+std::pow(a2y-vy,2))) * 0.3);
        int steps = 20;
        for (int i = 0; i <= steps; i++) {
            double t = ang1 + (ang2 - ang1) * i / steps;
            int px = vx + (int)(r * std::cos(t));
            int py = vy + (int)(r * std::sin(t));
            SDL_RenderDrawPoint(renderer, px, py);
        }

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f deg", aa.angleDeg);
        int tx = vx + (int)(r * 1.3 * std::cos((ang1+ang2)*0.5)) - 20;
        int ty = vy + (int)(r * 1.3 * std::sin((ang1+ang2)*0.5)) - 5;
        int tw = (int)std::strlen(buf) * 6 + 4;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_Rect bg = {tx - 2, ty - 2, tw, 14};
        SDL_RenderFillRect(renderer, &bg);
        DrawString(tx, ty, buf, 50, 200, 255);

        SDL_SetRenderDrawColor(renderer, 50, 200, 255, 255);
        SDL_Rect ca2 = {a2x-3, a2y-3, 6, 6};
        SDL_RenderDrawRect(renderer, &ca2);
    }

    SDL_SetRenderDrawColor(renderer, 50, 200, 255, 255);
    SDL_Rect ca1 = {a1x-3, a1y-3, 6, 6};
    SDL_RenderDrawRect(renderer, &ca1);
}

void DicomViewerSDL::RenderROIOverlay(const ROIAnnotation& ra) {
    auto* renderer = (SDL_Renderer*)sdlRenderer_;
    if (!renderer) return;
    int winW, winH;
    SDL_GetWindowSize((SDL_Window*)sdlWindow_, &winW, &winH);
    double ox = (winW - dicomWidth_ * zoomFactor_) * 0.5 + panX_;
    double oy = (winH - dicomHeight_ * zoomFactor_) * 0.5 + panY_;

    int x1 = (int)(std::min(ra.corner1.x, ra.corner2.x) * zoomFactor_ + ox);
    int y1 = (int)(std::min(ra.corner1.y, ra.corner2.y) * zoomFactor_ + oy);
    int x2 = (int)(std::max(ra.corner1.x, ra.corner2.x) * zoomFactor_ + ox);
    int y2 = (int)(std::max(ra.corner1.y, ra.corner2.y) * zoomFactor_ + oy);

    SDL_Rect rect = {x1, y1, x2 - x1, y2 - y1};

    SDL_SetRenderDrawColor(renderer, 255, 255, 50, 255);
    SDL_RenderDrawRect(renderer, &rect);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 50, 30);
    SDL_RenderFillRect(renderer, &rect);

    if (ra.complete) {
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Min:%.0f Max:%.0f Mean:%.1f",
                      ra.minVal, ra.maxVal, ra.meanVal);
        int tx = x1 + 4, ty = y1 + 4;
        int tw = (int)std::strlen(buf) * 6 + 4;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_Rect bg = {tx - 2, ty - 2, tw, 14};
        SDL_RenderFillRect(renderer, &bg);
        DrawString(tx, ty, buf, 255, 255, 50);
    }
}

void DicomViewerSDL::RenderTextOverlay(const TextAnnotation& ta) {
    auto* renderer = (SDL_Renderer*)sdlRenderer_;
    if (!renderer) return;
    int winW, winH;
    SDL_GetWindowSize((SDL_Window*)sdlWindow_, &winW, &winH);
    double ox = (winW - dicomWidth_ * zoomFactor_) * 0.5 + panX_;
    double oy = (winH - dicomHeight_ * zoomFactor_) * 0.5 + panY_;

    int sx = (int)(ta.pos.x * zoomFactor_ + ox);
    int sy = (int)(ta.pos.y * zoomFactor_ + oy);

    int tw = (int)ta.text.length() * 6 + 4;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_Rect bg = {sx, sy, tw, 14};
    SDL_RenderFillRect(renderer, &bg);

    SDL_SetRenderDrawColor(renderer, ta.r, ta.g, ta.b, 255);
    SDL_Rect cursor = {sx, sy, tw, 1};
    SDL_RenderFillRect(renderer, &cursor);

    DrawString(sx + 2, sy + 1, ta.text.c_str(), ta.r, ta.g, ta.b);
}

void DicomViewerSDL::RenderAnnotations() {
    for (auto& d : distances_) RenderDistanceOverlay(d);
    for (auto& a : angles_) RenderAngleOverlay(a);
    for (auto& r : rois_) RenderROIOverlay(r);
    for (auto& t : texts_) RenderTextOverlay(t);

    if (placingAnnotation_) {
        switch (currentMode_) {
            case AnnotationMode::DISTANCE:
                if (pendingDistance_.p1.x != 0 || pendingDistance_.p1.y != 0)
                    RenderDistanceOverlay(pendingDistance_);
                break;
            case AnnotationMode::ANGLE:
                RenderAngleOverlay(pendingAngle_);
                break;
            case AnnotationMode::ROI:
                RenderROIOverlay(pendingROI_);
                break;
            case AnnotationMode::TEXT:
                RenderTextOverlay(pendingText_);
                break;
            default: break;
        }
    }
}

// ============================================================
// Render frame
// ============================================================
void DicomViewerSDL::RenderFrame() {
    if (!sdlRenderer_ || !sdlTexture_ || !imageLoaded_) return;
    auto* renderer = (SDL_Renderer*)sdlRenderer_;
    auto* texture = (SDL_Texture*)sdlTexture_;
    auto* window = (SDL_Window*)sdlWindow_;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    ExpandToRGBA(pixelData_.data(), dicomWidth_, dicomHeight_, rgbaBuffer_);
    SDL_UpdateTexture(texture, nullptr, rgbaBuffer_.data(), dicomWidth_ * 4);

    int winW, winH;
    SDL_GetWindowSize(window, &winW, &winH);
    float sW = dicomWidth_ * zoomFactor_;
    float sH = dicomHeight_ * zoomFactor_;
    SDL_Rect dst;
    dst.w = (int)sW; dst.h = (int)sH;
    dst.x = (int)((winW - sW) * 0.5f + panX_);
    dst.y = (int)((winH - sH) * 0.5f + panY_);
    SDL_RenderCopy(renderer, texture, nullptr, &dst);

    RenderAnnotations();

    // --- HUD overlay ---
    int hudH = 56;
    if (seriesMode_) hudH = 78;
    SDL_Rect topBar = {0, 0, winW, hudH};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_RenderFillRect(renderer, &topBar);

    int x0 = 8, y0 = 4;
    // Mode indicator
    const char* modeStr = "WL";
    unsigned char mr=100, mg=255, mb=100;
    switch (currentMode_) {
        case AnnotationMode::DISTANCE: modeStr = "Dist"; mr=255; mg=80; mb=80; break;
        case AnnotationMode::ANGLE:    modeStr = "Ang";  mr=50;  mg=200; mb=255; break;
        case AnnotationMode::ROI:      modeStr = "ROI";  mr=255; mg=255; mb=50;  break;
        case AnnotationMode::TEXT:     modeStr = "Text"; mr=200; mg=100; mb=255; break;
        default: break;
    }
    SDL_SetRenderDrawColor(renderer, mr, mg, mb, 220);
    SDL_Rect modeBg = {x0, y0, 60, 18};
    SDL_RenderFillRect(renderer, &modeBg);
    DrawString(x0 + 2, y0 + 1, modeStr, 0, 0, 0);

    // Series info
    if (seriesMode_) {
        char sInfo[64];
        std::snprintf(sInfo, sizeof(sInfo), "%d/%d", currentSeriesIndex_ + 1, (int)instanceIds_.size());
        SDL_SetRenderDrawColor(renderer, 100, 180, 255, 220);
        SDL_Rect rs = {x0 + 70, y0, 70, 18};
        SDL_RenderFillRect(renderer, &rs);
        DrawString(x0 + 72, y0 + 1, sInfo, 0, 0, 0);
    }

    // Dims
    int dimX = seriesMode_ ? 150 : 180;
    SDL_SetRenderDrawColor(renderer, 60, 180, 255, 220);
    SDL_Rect r1 = {x0 + (seriesMode_ ? 150 : 70), y0, dimX, 18};
    SDL_RenderFillRect(renderer, &r1);
    // Zoom
    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 220);
    SDL_Rect r2 = {x0 + (seriesMode_ ? 310 : 260), y0, 100, 18};
    SDL_RenderFillRect(renderer, &r2);
    // WL
    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 220);
    SDL_Rect r3 = {x0 + (seriesMode_ ? 420 : 370), y0, 200, 18};
    SDL_RenderFillRect(renderer, &r3);

    // Filter indicator
    const char* filterNames[] = {"Orig","Equal","Smth","Edge"};
    unsigned char fc[][3] = {{180,180,180},{255,100,200},{100,200,255},{255,200,50}};
    int fi = (int)currentFilter_;
    int fx = x0 + (seriesMode_ ? 630 : 580);
    SDL_SetRenderDrawColor(renderer, fc[fi][0], fc[fi][1], fc[fi][2], 220);
    SDL_Rect rf = {fx, y0, 64, 18};
    SDL_RenderFillRect(renderer, &rf);
    DrawString(fx+2, y0+1, filterNames[fi], 0, 0, 0);

    // Zoom bar
    int zbw = (int)((zoomFactor_ / 5.0f) * (winW - 20));
    if (zbw < 4) zbw = 4;
    if (zbw > winW-20) zbw = winW-20;
    SDL_SetRenderDrawColor(renderer, 255, 200, 50, 200);
    SDL_Rect r4 = {x0, y0+22, zbw, 6};
    SDL_RenderFillRect(renderer, &r4);

    // WL bar
    if (windowWidth_ > 0) {
        double nw = std::min(windowWidth_ / 500.0, 1.0);
        int wbw = (int)(nw * (winW - 20));
        if (wbw < 4) wbw = 4;
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 200);
        SDL_Rect r5 = {x0, y0+32, wbw, 6};
        SDL_RenderFillRect(renderer, &r5);
    }

    // Bottom bar
    int botH = 22;
    SDL_Rect botBar = {0, winH-botH, winW, botH};
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderFillRect(renderer, &botBar);

    // Bottom controls
    int cx = 8;
    DrawString(cx, winH-18, "W:WL", 100, 255, 100);
    cx = winW/7;
    DrawString(cx, winH-18, "D:Dist", 255, 80, 80);
    cx = winW*2/7;
    DrawString(cx, winH-18, "A:Ang", 50, 200, 255);
    cx = winW*3/7;
    DrawString(cx, winH-18, "O:ROI", 255, 255, 50);
    cx = winW*4/7;
    DrawString(cx, winH-18, "T:Text", 200, 100, 255);
    cx = winW*5/7;
    DrawString(cx, winH-18, "1:Orig 2:Eq 3:Smt 4:Edg", 180, 180, 180);
    cx = winW*6/7;
    DrawString(cx, winH-18, "ESC:Salir", 200, 200, 200);

    // Separators
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 200);
    for (int i = 1; i < 7; i++)
        SDL_RenderDrawLine(renderer, winW*i/7, winH-botH, winW*i/7, winH);

    // Series nav arrows (in bottom bar)
    if (seriesMode_) {
        DrawString(winW/2 - 60, winH-18, "< >", 100, 180, 255);
    }

    // Mode help text
    if (placingAnnotation_) {
        const char* help = nullptr;
        switch (currentMode_) {
            case AnnotationMode::DISTANCE: help = "Click: pto1 -> pto2 | ClickDer: cancelar | Enter: fijar"; break;
            case AnnotationMode::ANGLE:    help = "Click: vertice -> brazo1 -> brazo2"; break;
            case AnnotationMode::ROI:      help = "Click-arrastrar: definir ROI"; break;
            case AnnotationMode::TEXT:     help = "Click: posicion | Tipear texto | Enter: fijar | Esc: cancelar"; break;
            default: break;
        }
        if (help) {
            SDL_Rect helpBar = {0, winH - 44, winW, 22};
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_RenderFillRect(renderer, &helpBar);
            DrawString(8, winH-40, help, 200, 200, 200);
        }
    }

    // Annotation count
    char ac[64];
    std::snprintf(ac, sizeof(ac), "D:%zu A:%zu R:%zu T:%zu",
                  distances_.size(), angles_.size(), rois_.size(), texts_.size());
    DrawString(winW - 150, winH-18, ac, 180, 180, 180);

    // Undo/Redo hint
    if (!undoStack_.empty() || !redoStack_.empty()) {
        DrawString(winW - 260, winH-18, "CZ:Undo CY:Redo", 120, 120, 120);
    }

    SDL_RenderPresent(renderer);
}

// ============================================================
// Apply image filter (reads basePixelData_, writes pixelData_)
// ============================================================
void DicomViewerSDL::ApplyFilter(ImageFilter filter) {
    currentFilter_ = filter;
    if (filter == ImageFilter::NONE) {
        pixelData_ = basePixelData_;
        return;
    }
    if (basePixelData_.empty() || dicomWidth_ == 0 || dicomHeight_ == 0) return;

    // Copy base before modifying
    pixelData_ = basePixelData_;

    switch (filter) {
        case ImageFilter::HISTOGRAM_EQUALIZE: ApplyHistogramEqualize(); break;
        case ImageFilter::SMOOTH:             ApplySmooth();            break;
        case ImageFilter::EDGE_DETECT:        ApplyEdgeDetect();       break;
        default: break;
    }
}

// ============================================================
// Histogram Equalization
// ============================================================
void DicomViewerSDL::ApplyHistogramEqualize() {
    unsigned long n = (unsigned long)dicomWidth_ * dicomHeight_;
    if (n == 0) return;

    // Step 1: Compute histogram (256 bins for 8-bit)
    int hist[256] = {0};
    for (unsigned long i = 0; i < n; i++) {
        hist[pixelData_[i]]++;
    }

    // Step 2: Compute CDF (Cumulative Distribution Function)
    unsigned long cdf[256];
    cdf[0] = hist[0];
    for (int i = 1; i < 256; i++) {
        cdf[i] = cdf[i-1] + hist[i];
    }

    // Step 3: Find first non-zero CDF bin
    unsigned long cdfMin = 0;
    for (int i = 0; i < 256; i++) {
        if (cdf[i] > 0) { cdfMin = cdf[i]; break; }
    }
    if (cdfMin == 0 || n - cdfMin == 0) return; // all pixels same value

    // Step 4: Remap pixels using equalized LUT
    unsigned char lut[256];
    double scale = 255.0 / (n - cdfMin);
    for (int i = 0; i < 256; i++) {
        unsigned long num = cdf[i] - cdfMin;
        int v = (int)(num * scale + 0.5);
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        lut[i] = (unsigned char)v;
    }

    for (unsigned long i = 0; i < n; i++) {
        pixelData_[i] = lut[pixelData_[i]];
    }
}

// ============================================================
// Smoothing: 3x3 Box Blur
// ============================================================
void DicomViewerSDL::ApplySmooth() {
    unsigned long w = dicomWidth_;
    unsigned long h = dicomHeight_;
    if (w == 0 || h == 0) return;

    std::vector<unsigned char> result(basePixelData_);

    static const int kernel[3][3] = {
        {1, 1, 1},
        {1, 1, 1},
        {1, 1, 1}
    };

    for (unsigned long iy = 0; iy < h; iy++) {
        for (unsigned long ix = 0; ix < w; ix++) {
            int sum = 0;
            int count = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    long sy = (long)iy + ky;
                    long sx = (long)ix + kx;
                    if (sy >= 0 && sy < (long)h && sx >= 0 && sx < (long)w) {
                        sum += basePixelData_[sy * w + sx] * kernel[ky+1][kx+1];
                        count += kernel[ky+1][kx+1];
                    }
                }
            }
            result[iy * w + ix] = (unsigned char)(sum / count);
        }
    }

    pixelData_.swap(result);
}

// ============================================================
// Edge Detection: Sobel operator (3x3)
// ============================================================
void DicomViewerSDL::ApplyEdgeDetect() {
    unsigned long w = dicomWidth_;
    unsigned long h = dicomHeight_;
    if (w == 0 || h == 0) return;

    std::vector<unsigned char> result(basePixelData_);

    static const int sobelX[3][3] = {
        {-1,  0,  1},
        {-2,  0,  2},
        {-1,  0,  1}
    };
    static const int sobelY[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    for (unsigned long iy = 1; iy < h - 1; iy++) {
        for (unsigned long ix = 1; ix < w - 1; ix++) {
            int gx = 0, gy = 0;
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    unsigned char p = basePixelData_[(iy + ky) * w + (ix + kx)];
                    gx += p * sobelX[ky+1][kx+1];
                    gy += p * sobelY[ky+1][kx+1];
                }
            }
            // Magnitude, clamped to [0, 255]
            int mag = (int)(std::sqrt((double)(gx*gx + gy*gy)) + 0.5);
            if (mag < 0) mag = 0;
            if (mag > 255) mag = 255;
            result[iy * w + ix] = (unsigned char)mag;
        }
    }

    pixelData_.swap(result);
}

// ============================================================
// Set mode
// ============================================================
void DicomViewerSDL::SetMode(AnnotationMode mode) {
    if (textInputActive_) {
        FinishTextAnnotation();
    }
    CancelPendingAnnotation();
    currentMode_ = mode;

    if (mode == AnnotationMode::TEXT) {
        SDL_StartTextInput();
    } else {
        SDL_StopTextInput();
    }
}

// ============================================================
// Place annotation point
// ============================================================
void DicomViewerSDL::PlaceAnnotationPoint(int sx, int sy) {
    ImagePoint ip = ScreenToImage(sx, sy);
    if (ip.x < 0) ip.x = 0;
    if (ip.x >= dicomWidth_) ip.x = dicomWidth_ - 1;
    if (ip.y < 0) ip.y = 0;
    if (ip.y >= dicomHeight_) ip.y = dicomHeight_ - 1;

    switch (currentMode_) {
        case AnnotationMode::DISTANCE: {
            PushUndoState();
            if (!placingAnnotation_) {
                pendingDistance_ = DistanceAnnotation();
                pendingDistance_.pixelSpacing = (pixelSpacingX_ + pixelSpacingY_) * 0.5;
                pendingDistance_.p1 = ip;
                pendingDistance_.complete = false;
                placingAnnotation_ = true;
            } else {
                pendingDistance_.p2 = ip;
                double dx = pendingDistance_.p2.x - pendingDistance_.p1.x;
                double dy = pendingDistance_.p2.y - pendingDistance_.p1.y;
                double distPx = std::sqrt(dx*dx + dy*dy);
                pendingDistance_.distanceMM = distPx * pendingDistance_.pixelSpacing;
                pendingDistance_.complete = true;
                distances_.push_back(pendingDistance_);
                placingAnnotation_ = false;
            }
            break;
        }
        case AnnotationMode::ANGLE: {
            PushUndoState();
            if (!placingAnnotation_) {
                pendingAngle_ = AngleAnnotation();
                pendingAngle_.vertex = ip;
                pendingAngle_.pointsPlaced = 1;
                placingAnnotation_ = true;
            } else if (pendingAngle_.pointsPlaced == 1) {
                pendingAngle_.arm1 = ip;
                pendingAngle_.pointsPlaced = 2;
            } else if (pendingAngle_.pointsPlaced == 2) {
                pendingAngle_.arm2 = ip;
                pendingAngle_.pointsPlaced = 3;

                double v1x = pendingAngle_.arm1.x - pendingAngle_.vertex.x;
                double v1y = pendingAngle_.arm1.y - pendingAngle_.vertex.y;
                double v2x = pendingAngle_.arm2.x - pendingAngle_.vertex.x;
                double v2y = pendingAngle_.arm2.y - pendingAngle_.vertex.y;
                double dot = v1x*v2x + v1y*v2y;
                double n1 = std::sqrt(v1x*v1x + v1y*v1y);
                double n2 = std::sqrt(v2x*v2x + v2y*v2y);
                double cosAng = std::max(-1.0, std::min(1.0, dot / (n1*n2)));
                pendingAngle_.angleDeg = std::acos(cosAng) * 180.0 / M_PI;

                angles_.push_back(pendingAngle_);
                placingAnnotation_ = false;
            }
            break;
        }
        case AnnotationMode::ROI: {
            // Handled via drag
            break;
        }
        case AnnotationMode::TEXT: {
            if (!textInputActive_) {
                PushUndoState();
                pendingText_ = TextAnnotation(ip, "");
                placingAnnotation_ = true;
                textInputActive_ = true;
                SDL_StartTextInput();
            } else {
                FinishTextAnnotation();
            }
            break;
        }
        default: break;
    }
}

void DicomViewerSDL::FinishTextAnnotation() {
    if (textInputActive_ && !pendingText_.text.empty()) {
        pendingText_.text.erase(std::remove(pendingText_.text.begin(),
            pendingText_.text.end(), '\n'), pendingText_.text.end());
        texts_.push_back(pendingText_);
    }
    textInputActive_ = false;
    placingAnnotation_ = false;
    pendingText_ = TextAnnotation();
    SDL_StopTextInput();
}

void DicomViewerSDL::CancelPendingAnnotation() {
    if (textInputActive_) {
        SDL_StopTextInput();
        textInputActive_ = false;
    }
    placingAnnotation_ = false;
    pendingDistance_ = DistanceAnnotation();
    pendingAngle_ = AngleAnnotation();
    pendingROI_ = ROIAnnotation();
    pendingText_ = TextAnnotation();
}

// ============================================================
// Handle text input (SDL_TEXTINPUT)
// ============================================================
void DicomViewerSDL::HandleTextInput(const char* text) {
    if (!textInputActive_ || !placingAnnotation_) return;
    pendingText_.text += text;
}

// ============================================================
// Handle key
// ============================================================
void DicomViewerSDL::HandleKey(int key, unsigned short mod) {
    SDL_Keycode k = (SDL_Keycode)key;
    bool ctrl = (mod & KMOD_CTRL);

    // Ctrl+Z = Undo
    if (ctrl && k == SDLK_z) {
        Undo();
        return;
    }
    // Ctrl+Y or Ctrl+Shift+Z = Redo
    if ((ctrl && k == SDLK_y) || (ctrl && (mod & KMOD_SHIFT) && k == SDLK_z)) {
        Redo();
        return;
    }
    // Ctrl+E = Export SR
    if (ctrl && k == SDLK_e) {
        ExportSR("/tmp/orthanc_annotations_sr.dcm");
        return;
    }

    if (textInputActive_) {
        if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
            FinishTextAnnotation();
        } else if (k == SDLK_BACKSPACE && !pendingText_.text.empty()) {
            pendingText_.text.pop_back();
        } else if (k == SDLK_ESCAPE) {
            CancelPendingAnnotation();
        }
        return;
    }

    // Series navigation
    if (seriesMode_) {
        if (k == SDLK_LEFT || k == SDLK_UP) {
            if (currentSeriesIndex_ > 0) {
                LoadInstance(currentSeriesIndex_ - 1);
                RenderWithWindowLevel(0, 0);
            }
            return;
        }
        if (k == SDLK_RIGHT || k == SDLK_DOWN) {
            if (currentSeriesIndex_ < (int)instanceIds_.size() - 1) {
                LoadInstance(currentSeriesIndex_ + 1);
                RenderWithWindowLevel(0, 0);
            }
            return;
        }
    }

    // Filter switching (1-4)
    if (k >= SDLK_1 && k <= SDLK_4) {
        static const ImageFilter filterMap[] = {
            ImageFilter::NONE,
            ImageFilter::HISTOGRAM_EQUALIZE,
            ImageFilter::SMOOTH,
            ImageFilter::EDGE_DETECT
        };
        int idx = k - SDLK_1;
        if (idx >= 0 && idx <= 3) {
            ApplyFilter(filterMap[idx]);
        }
        return;
    }

    // Mode switching / actions
    switch (k) {
        case SDLK_r: ResetView(); break;
        case SDLK_w: SetMode(AnnotationMode::WL); break;
        case SDLK_d: SetMode(AnnotationMode::DISTANCE); break;
        case SDLK_a: SetMode(AnnotationMode::ANGLE); break;
        case SDLK_o: SetMode(AnnotationMode::ROI); break;
        case SDLK_t: SetMode(AnnotationMode::TEXT); break;
        case SDLK_PLUS:
        case SDLK_EQUALS: zoomFactor_ *= 1.2f; if (zoomFactor_ > 50.0f) zoomFactor_ = 50.0f; break;
        case SDLK_MINUS: zoomFactor_ /= 1.2f; if (zoomFactor_ < 0.1f) zoomFactor_ = 0.1f; break;
        case SDLK_0: zoomFactor_ = 1.0f; break;
        case SDLK_DELETE:
        case SDLK_BACKSPACE:
            PushUndoState();
            switch (currentMode_) {
                case AnnotationMode::DISTANCE: if (!distances_.empty()) distances_.pop_back(); break;
                case AnnotationMode::ANGLE:    if (!angles_.empty()) angles_.pop_back(); break;
                case AnnotationMode::ROI:      if (!rois_.empty()) rois_.pop_back(); break;
                case AnnotationMode::TEXT:     if (!texts_.empty()) texts_.pop_back(); break;
                default: break;
            }
            break;
        default: break;
    }
}

// ============================================================
// Event handlers
// ============================================================
void DicomViewerSDL::HandleMouseMotion(int x, int y) {
    int dx = x - lastMouseX_, dy = y - lastMouseY_;
    lastMouseX_ = x; lastMouseY_ = y;

    if (mouseDraggingWL_) {
        if (windowWidth_ <= 0) windowWidth_ = 50.0;
        windowWidth_  += dx * 1.5 * (windowWidth_ / 100.0);
        if (windowWidth_ < 1.0) windowWidth_ = 1.0;
        windowCenter_ -= dy * 1.5;
        autoWindowLevel_ = false;
        RenderWithWindowLevel(windowCenter_, windowWidth_);
    } else if (mouseDraggingPan_) {
        panX_ += dx; panY_ += dy;
    } else if (mouseDraggingROI_) {
        pendingROI_.corner2 = ScreenToImage(x, y);
    }

    if (placingAnnotation_ && !mouseDraggingWL_ && !mouseDraggingPan_ && !mouseDraggingROI_) {
        ImagePoint ip = ScreenToImage(x, y);
        switch (currentMode_) {
            case AnnotationMode::DISTANCE:
                if (pendingDistance_.p1.x != 0 || pendingDistance_.p1.y != 0)
                    pendingDistance_.p2 = ip;
                break;
            case AnnotationMode::ANGLE:
                if (pendingAngle_.pointsPlaced == 2)
                    pendingAngle_.arm2 = ip;
                break;
            default: break;
        }
    }
}

void DicomViewerSDL::HandleMouseButton(int button, bool pressed, int x, int y) {
    if (pressed) {
        lastMouseX_ = x; lastMouseY_ = y;

        if (button == SDL_BUTTON_LEFT) {
            if (currentMode_ == AnnotationMode::WL) {
                mouseDraggingWL_ = true;
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
            } else if (currentMode_ == AnnotationMode::DISTANCE ||
                       currentMode_ == AnnotationMode::ANGLE ||
                       currentMode_ == AnnotationMode::TEXT) {
                PlaceAnnotationPoint(x, y);
            } else if (currentMode_ == AnnotationMode::ROI) {
                mouseDraggingROI_ = true;
                pendingROI_ = ROIAnnotation();
                pendingROI_.corner1 = ScreenToImage(x, y);
                pendingROI_.corner2 = pendingROI_.corner1;
                placingAnnotation_ = true;
            }
        } else if (button == SDL_BUTTON_RIGHT) {
            if (currentMode_ != AnnotationMode::WL) {
                CancelPendingAnnotation();
            } else {
                mouseDraggingPan_ = true;
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL));
            }
        }
    } else {
        if (button == SDL_BUTTON_LEFT) {
            if (mouseDraggingWL_) {
                mouseDraggingWL_ = false;
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
            }
            if (mouseDraggingROI_) {
                mouseDraggingROI_ = false;
                if (pendingROI_.corner1.x != pendingROI_.corner2.x &&
                    pendingROI_.corner1.y != pendingROI_.corner2.y) {
                    PushUndoState();
                    pendingROI_.complete = true;
                    ComputeROIStats(pendingROI_);
                    rois_.push_back(pendingROI_);
                }
                placingAnnotation_ = false;
            }
        } else if (button == SDL_BUTTON_RIGHT) {
            if (mouseDraggingPan_) {
                mouseDraggingPan_ = false;
                SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
            }
        }
    }
}

void DicomViewerSDL::HandleMouseWheel(int delta) {
    if (delta > 0) zoomFactor_ *= 1.1f;
    else if (delta < 0) zoomFactor_ /= 1.1f;
    if (zoomFactor_ < 0.1f) zoomFactor_ = 0.1f;
    if (zoomFactor_ > 50.0f) zoomFactor_ = 50.0f;
}

void DicomViewerSDL::ResetView() {
    zoomFactor_ = 1.0f; panX_ = 0.0; panY_ = 0.0;
    windowCenter_ = 0.0; windowWidth_ = 0.0;
    autoWindowLevel_ = true;
    RenderWithWindowLevel(0, 0);
}

void DicomViewerSDL::UpdateWindowTitle() {
    if (!sdlWindow_) return;
    char t[640];
    const char* modeNames[] = {"WL","Dist","Ang","ROI","Text"};
    const char* filterNames[] = {""," HEq"," Smth"," Edge"};
    if (seriesMode_) {
        std::snprintf(t, sizeof(t), "%s | [%s]%s %d/%d %lux%lu | Z:%.1f | WL:%.0f/%.0f",
                      windowTitle_.c_str(), modeNames[(int)currentMode_],
                      filterNames[(int)currentFilter_],
                      currentSeriesIndex_ + 1, (int)instanceIds_.size(),
                      dicomWidth_, dicomHeight_,
                      zoomFactor_, windowCenter_, windowWidth_);
    } else {
        std::snprintf(t, sizeof(t), "%s | [%s]%s %lux%lu | Z:%.1f | WL:%.0f/%.0f",
                      windowTitle_.c_str(), modeNames[(int)currentMode_],
                      filterNames[(int)currentFilter_],
                      dicomWidth_, dicomHeight_,
                      zoomFactor_, windowCenter_, windowWidth_);
    }
    SDL_SetWindowTitle((SDL_Window*)sdlWindow_, t);
}

// ============================================================
// SR Export
// ============================================================
bool DicomViewerSDL::ExportSR(const std::string& outputPath) {
    auto* ff = static_cast<DcmFileFormat*>(dicomDataset_.get());
    if (!ff) {
        std::cerr << "  [ERROR] No hay DICOM cargado\n";
        return false;
    }
    DcmDataset* srcDs = ff->getDataset();

    // Read source identifiers
    OFString patientName, patientId, studyUid, seriesUid, sopUid;
    OFString studyDate, studyTime, studyDesc;
    srcDs->findAndGetOFString(DCM_PatientName, patientName);
    srcDs->findAndGetOFString(DCM_PatientID, patientId);
    srcDs->findAndGetOFString(DCM_StudyInstanceUID, studyUid);
    srcDs->findAndGetOFString(DCM_SeriesInstanceUID, seriesUid);
    srcDs->findAndGetOFString(DCM_SOPInstanceUID, sopUid);
    srcDs->findAndGetOFString(DCM_StudyDate, studyDate);
    srcDs->findAndGetOFString(DCM_StudyTime, studyTime);
    srcDs->findAndGetOFString(DCM_StudyDescription, studyDesc);

    // =========================================================
    // Build SR dataset manually using dcmdata
    // =========================================================
    DcmFileFormat outFf;
    DcmDataset* ds = outFf.getDataset();

    // SOP Common
    ds->putAndInsertString(DCM_SOPClassUID, UID_BasicTextSRStorage);
    char uidBuf[128];
    dcmGenerateUniqueIdentifier(uidBuf);
    ds->putAndInsertString(DCM_SOPInstanceUID, uidBuf);
    ds->putAndInsertString(DCM_SpecificCharacterSet, "ISO_IR 100");

    // Patient
    if (!patientName.empty()) ds->putAndInsertString(DCM_PatientName, patientName.c_str());
    if (!patientId.empty())   ds->putAndInsertString(DCM_PatientID, patientId.c_str());

    // Study
    if (!studyUid.empty())  ds->putAndInsertString(DCM_StudyInstanceUID, studyUid.c_str());
    if (!studyDate.empty()) ds->putAndInsertString(DCM_StudyDate, studyDate.c_str());
    if (!studyTime.empty()) ds->putAndInsertString(DCM_StudyTime, studyTime.c_str());
    if (!studyDesc.empty()) ds->putAndInsertString(DCM_StudyDescription, studyDesc.c_str());

    // Series
    char seriesUidBuf[128];
    dcmGenerateUniqueIdentifier(seriesUidBuf);
    ds->putAndInsertString(DCM_SeriesInstanceUID, seriesUidBuf);
    ds->putAndInsertString(DCM_SeriesNumber, "999");
    ds->putAndInsertString(DCM_Modality, "SR");

    // SR Document
    ds->putAndInsertString(DCM_CompletionFlag, "COMPLETE");
    ds->putAndInsertString(DCM_VerificationFlag, "UNVERIFIED");

    // =========================================================
    // Content Sequence (0040,A730)
    // =========================================================
    DcmSequenceOfItems* cs = new DcmSequenceOfItems(DCM_ContentSequence);

    // --- ROOT CONTAINER ---
    {
        DcmItem* rootItem = new DcmItem();
        rootItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
        rootItem->putAndInsertString(DCM_ValueType, "CONTAINER");
        rootItem->putAndInsertString(DCM_ContinuityOfContent, "SEPARATE");

        // Concept name: (126000, DCM, "Imaging Measurement Report")
        DcmSequenceOfItems* cncs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
        DcmItem* cni = new DcmItem();
        cni->putAndInsertString(DCM_CodeValue, "126000");
        cni->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
        cni->putAndInsertString(DCM_CodeMeaning, "Imaging Measurement Report");
        cncs->insert(cni);
        rootItem->insert(cncs);

        // Image reference (first reference to source)
        DcmSequenceOfItems* refSeq = new DcmSequenceOfItems(DCM_ReferencedSOPSequence);
        DcmItem* refItem = new DcmItem();
        if (!sopUid.empty()) {
            refItem->putAndInsertString(DCM_ReferencedSOPClassUID, UID_CTImageStorage);
            refItem->putAndInsertString(DCM_ReferencedSOPInstanceUID, sopUid.c_str());
        }
        refSeq->insert(refItem);
        rootItem->insert(refSeq);

        // Children content sequence
        DcmSequenceOfItems* children = new DcmSequenceOfItems(DCM_ContentSequence);


        // --- Export distances ---
        for (size_t i = 0; i < distances_.size(); i++) {
            if (!distances_[i].complete) continue;
            auto& d = distances_[i];

            // Finding container
            DcmItem* findItem = new DcmItem();
            findItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            findItem->putAndInsertString(DCM_ValueType, "CONTAINER");

            DcmSequenceOfItems* fcs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* fci = new DcmItem();
            fci->putAndInsertString(DCM_CodeValue, "121071");
            fci->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            fci->putAndInsertString(DCM_CodeMeaning, "Finding");
            fcs->insert(fci);
            findItem->insert(fcs);

            // Finding children
            DcmSequenceOfItems* findChildren = new DcmSequenceOfItems(DCM_ContentSequence);

            // NUM: Length
            DcmItem* numItem = new DcmItem();
            numItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            numItem->putAndInsertString(DCM_ValueType, "NUM");
            DcmSequenceOfItems* ncs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* nci = new DcmItem();
            nci->putAndInsertString(DCM_CodeValue, "G-A220");
            nci->putAndInsertString(DCM_CodingSchemeDesignator, "SRT");
            nci->putAndInsertString(DCM_CodeMeaning, "Length");
            ncs->insert(nci);
            numItem->insert(ncs);
            char vb[64];
            std::snprintf(vb, sizeof(vb), "%.2f", d.distanceMM);
            numItem->putAndInsertString(DCM_NumericValue, vb);
            DcmSequenceOfItems* mcs = new DcmSequenceOfItems(DCM_MeasuredValueSequence);
            DcmItem* mci = new DcmItem();
            DcmSequenceOfItems* ucs = new DcmSequenceOfItems(DCM_MeasurementUnitsCodeSequence);
            DcmItem* uci = new DcmItem();
            uci->putAndInsertString(DCM_CodeValue, "mm");
            uci->putAndInsertString(DCM_CodingSchemeDesignator, "UCUM");
            uci->putAndInsertString(DCM_CodeMeaning, "millimeter");
            ucs->insert(uci);
            mci->insert(ucs);
            mcs->insert(mci);
            numItem->insert(mcs);
            findChildren->insert(numItem);

            // SCOORD: POLYLINE (2 points)
            DcmItem* scItem = new DcmItem();
            scItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            scItem->putAndInsertString(DCM_ValueType, "SCOORD");
            DcmSequenceOfItems* scs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* scc = new DcmItem();
            scc->putAndInsertString(DCM_CodeValue, "111030");
            scc->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            scc->putAndInsertString(DCM_CodeMeaning, "Image region");
            scs->insert(scc);
            scItem->insert(scs);
            scItem->putAndInsertString(DCM_GraphicType, "POLYLINE");
            char gdBuf[128];
            std::snprintf(gdBuf, sizeof(gdBuf), "%.2f\\%.2f\\%.2f\\%.2f",
                          d.p1.x, d.p1.y, d.p2.x, d.p2.y);
            scItem->putAndInsertString(DCM_GraphicData, gdBuf);
            findChildren->insert(scItem);

            findItem->insert(findChildren);
            children->insert(findItem);
        }

        // --- Export angles ---
        for (size_t i = 0; i < angles_.size(); i++) {
            if (angles_[i].pointsPlaced < 3) continue;
            auto& a = angles_[i];

            DcmItem* findItem = new DcmItem();
            findItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            findItem->putAndInsertString(DCM_ValueType, "CONTAINER");
            DcmSequenceOfItems* fcs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* fci = new DcmItem();
            fci->putAndInsertString(DCM_CodeValue, "121071");
            fci->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            fci->putAndInsertString(DCM_CodeMeaning, "Finding");
            fcs->insert(fci);
            findItem->insert(fcs);

            DcmSequenceOfItems* findChildren = new DcmSequenceOfItems(DCM_ContentSequence);

            // NUM: Angle
            DcmItem* numItem = new DcmItem();
            numItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            numItem->putAndInsertString(DCM_ValueType, "NUM");
            DcmSequenceOfItems* ncs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* nci = new DcmItem();
            nci->putAndInsertString(DCM_CodeValue, "G-A22B");
            nci->putAndInsertString(DCM_CodingSchemeDesignator, "SRT");
            nci->putAndInsertString(DCM_CodeMeaning, "Angle");
            ncs->insert(nci);
            numItem->insert(ncs);
            char vb[64];
            std::snprintf(vb, sizeof(vb), "%.2f", a.angleDeg);
            numItem->putAndInsertString(DCM_NumericValue, vb);
            DcmSequenceOfItems* mcs = new DcmSequenceOfItems(DCM_MeasuredValueSequence);
            DcmItem* mci = new DcmItem();
            DcmSequenceOfItems* ucs = new DcmSequenceOfItems(DCM_MeasurementUnitsCodeSequence);
            DcmItem* uci = new DcmItem();
            uci->putAndInsertString(DCM_CodeValue, "deg");
            uci->putAndInsertString(DCM_CodingSchemeDesignator, "UCUM");
            uci->putAndInsertString(DCM_CodeMeaning, "degree");
            ucs->insert(uci);
            mci->insert(ucs);
            mcs->insert(mci);
            numItem->insert(mcs);
            findChildren->insert(numItem);

            // SCOORD: POLYLINE (3 points: vertex, arm1, arm2)
            DcmItem* scItem = new DcmItem();
            scItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            scItem->putAndInsertString(DCM_ValueType, "SCOORD");
            DcmSequenceOfItems* scs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* scc = new DcmItem();
            scc->putAndInsertString(DCM_CodeValue, "111030");
            scc->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            scc->putAndInsertString(DCM_CodeMeaning, "Image region");
            scs->insert(scc);
            scItem->insert(scs);
            scItem->putAndInsertString(DCM_GraphicType, "POLYLINE");
            char gdBuf[128];
            std::snprintf(gdBuf, sizeof(gdBuf), "%.2f\\%.2f\\%.2f\\%.2f\\%.2f\\%.2f",
                          a.vertex.x, a.vertex.y, a.arm1.x, a.arm1.y, a.arm2.x, a.arm2.y);
            scItem->putAndInsertString(DCM_GraphicData, gdBuf);
            findChildren->insert(scItem);

            findItem->insert(findChildren);
            children->insert(findItem);
        }

        // --- Export ROIs ---
        for (size_t i = 0; i < rois_.size(); i++) {
            if (!rois_[i].complete) continue;
            auto& r = rois_[i];

            DcmItem* findItem = new DcmItem();
            findItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            findItem->putAndInsertString(DCM_ValueType, "CONTAINER");
            DcmSequenceOfItems* fcs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* fci = new DcmItem();
            fci->putAndInsertString(DCM_CodeValue, "121071");
            fci->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            fci->putAndInsertString(DCM_CodeMeaning, "Finding");
            fcs->insert(fci);
            findItem->insert(fcs);

            DcmSequenceOfItems* findChildren = new DcmSequenceOfItems(DCM_ContentSequence);

            // Helper for NUM items
            auto addNumInFinding = [&](const char* codeVal, const char* meaning, double v) {
                DcmItem* ni = new DcmItem();
                ni->putAndInsertString(DCM_RelationshipType, "CONTAINS");
                ni->putAndInsertString(DCM_ValueType, "NUM");
                DcmSequenceOfItems* ncs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
                DcmItem* nci = new DcmItem();
                nci->putAndInsertString(DCM_CodeValue, codeVal);
                nci->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
                nci->putAndInsertString(DCM_CodeMeaning, meaning);
                ncs->insert(nci);
                ni->insert(ncs);
                char vb[64];
                std::snprintf(vb, sizeof(vb), "%.2f", v);
                ni->putAndInsertString(DCM_NumericValue, vb);
                DcmSequenceOfItems* mcs = new DcmSequenceOfItems(DCM_MeasuredValueSequence);
                DcmItem* mci = new DcmItem();
                DcmSequenceOfItems* ucs = new DcmSequenceOfItems(DCM_MeasurementUnitsCodeSequence);
                DcmItem* uci = new DcmItem();
                uci->putAndInsertString(DCM_CodeValue, "1");
                uci->putAndInsertString(DCM_CodingSchemeDesignator, "UCUM");
                uci->putAndInsertString(DCM_CodeMeaning, "no units");
                ucs->insert(uci);
                mci->insert(ucs);
                mcs->insert(mci);
                ni->insert(mcs);
                findChildren->insert(ni);
            };

            addNumInFinding("121401", "Minimum pixel value", r.minVal);
            addNumInFinding("121402", "Maximum pixel value", r.maxVal);
            addNumInFinding("121403", "Mean pixel value", r.meanVal);

            // SCOORD: POLYLINE (4 corners of rectangle)
            DcmItem* scItem = new DcmItem();
            scItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            scItem->putAndInsertString(DCM_ValueType, "SCOORD");
            DcmSequenceOfItems* scs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* scc = new DcmItem();
            scc->putAndInsertString(DCM_CodeValue, "111030");
            scc->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            scc->putAndInsertString(DCM_CodeMeaning, "Image region");
            scs->insert(scc);
            scItem->insert(scs);
            scItem->putAndInsertString(DCM_GraphicType, "POLYLINE");
            double x1 = std::min(r.corner1.x, r.corner2.x);
            double y1 = std::min(r.corner1.y, r.corner2.y);
            double x2 = std::max(r.corner1.x, r.corner2.x);
            double y2 = std::max(r.corner1.y, r.corner2.y);
            char gdBuf[256];
            std::snprintf(gdBuf, sizeof(gdBuf),
                "%.2f\\%.2f\\%.2f\\%.2f\\%.2f\\%.2f\\%.2f\\%.2f",
                x1, y1, x2, y1, x2, y2, x1, y2);
            scItem->putAndInsertString(DCM_GraphicData, gdBuf);
            findChildren->insert(scItem);

            findItem->insert(findChildren);
            children->insert(findItem);
        }

        // --- Export texts ---
        for (size_t i = 0; i < texts_.size(); i++) {
            auto& t = texts_[i];

            DcmItem* findItem = new DcmItem();
            findItem->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            findItem->putAndInsertString(DCM_ValueType, "CONTAINER");
            DcmSequenceOfItems* fcs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* fci = new DcmItem();
            fci->putAndInsertString(DCM_CodeValue, "121071");
            fci->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            fci->putAndInsertString(DCM_CodeMeaning, "Finding");
            fcs->insert(fci);
            findItem->insert(fcs);

            DcmSequenceOfItems* findChildren = new DcmSequenceOfItems(DCM_ContentSequence);

            // TEXT item
            DcmItem* ti = new DcmItem();
            ti->putAndInsertString(DCM_RelationshipType, "CONTAINS");
            ti->putAndInsertString(DCM_ValueType, "TEXT");
            DcmSequenceOfItems* tcs = new DcmSequenceOfItems(DCM_ConceptNameCodeSequence);
            DcmItem* tci = new DcmItem();
            tci->putAndInsertString(DCM_CodeValue, "121106");
            tci->putAndInsertString(DCM_CodingSchemeDesignator, "DCM");
            tci->putAndInsertString(DCM_CodeMeaning, "Text observation");
            tcs->insert(tci);
            ti->insert(tcs);
            ti->putAndInsertString(DCM_TextValue, t.text.c_str());

            findChildren->insert(ti);
            findItem->insert(findChildren);
            children->insert(findItem);
        }

        rootItem->insert(children);
        cs->insert(rootItem);
    }

    ds->insert(cs);

    // Save file
    OFCondition result = outFf.saveFile(outputPath.c_str(), EXS_LittleEndianExplicit);
    if (result.good()) {
        std::cout << "  \033[32mSR exportado: " << outputPath << "\033[0m\n";

        // Offer to upload to Orthanc
        std::cout << "  Subir a Orthanc? (s/N): ";
        std::string ans;
        std::getline(std::cin, ans);
        if (!ans.empty() && (ans[0] == 's' || ans[0] == 'S')) {
            std::ifstream ifs(outputPath, std::ios::binary);
            if (ifs.is_open()) {
                std::vector<char> srData((std::istreambuf_iterator<char>(ifs)),
                                          std::istreambuf_iterator<char>());
                ifs.close();
                CURL* curl = curl_easy_init();
                if (curl) {
                    std::string url = client_.GetBaseUrl() + "/instances";
                    std::string resp;
                    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                    curl_easy_setopt(curl, CURLOPT_POST, 1L);
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, srData.data());
                    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, srData.size());
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToVecCb);
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp);
                    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
                    CURLcode cr = curl_easy_perform(curl);
                    long httpCode = 0;
                    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
                    curl_easy_cleanup(curl);
                    if (cr == CURLE_OK && httpCode < 400) {
                        std::cout << "  \033[32mSR subido a Orthanc\033[0m\n";
                    } else {
                        std::cout << "  \033[31mError HTTP " << httpCode << " al subir SR\033[0m\n";
                    }
                }
            }
        }
        return true;
    } else {
        std::cerr << "  [ERROR] " << result.text() << "\n";
        return false;
    }
}

// ============================================================
// Cleanup
// ============================================================
void DicomViewerSDL::CleanupSDL() {
    SDL_StopTextInput();
    if (sdlTexture_)  { SDL_DestroyTexture((SDL_Texture*)sdlTexture_);   sdlTexture_ = nullptr; }
    if (sdlRenderer_) { SDL_DestroyRenderer((SDL_Renderer*)sdlRenderer_); sdlRenderer_ = nullptr; }
    if (sdlWindow_)   { SDL_DestroyWindow((SDL_Window*)sdlWindow_);       sdlWindow_ = nullptr; }
    SDL_Quit();
}

// ============================================================
// Main loop
// ============================================================
int DicomViewerSDL::Run() {
    if (!LoadDicomFromOrthanc()) {
        std::cerr << "  [ERROR] No se pudo cargar la imagen\n";
        std::cout << "\n  Presione Enter..."; std::string d; std::getline(std::cin, d);
        return -1;
    }

    int ww = (int)dicomWidth_, wh = (int)dicomHeight_;
    const int mw=1400, mh=1000;
    if (ww>mw||wh>mh) { float s=std::min((float)mw/ww,(float)mh/wh); ww=(int)(ww*s); wh=(int)(wh*s); }
    if (ww<640) ww=640;
    if (wh<480) wh=480;

    if (!InitSDL(ww,wh)) return -1;

    bool running = true;
    SDL_Event ev;
    while (running) {
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT: running = false; break;
                case SDL_KEYDOWN:
                    if (ev.key.keysym.sym == SDLK_ESCAPE || ev.key.keysym.sym == SDLK_q) {
                        if (placingAnnotation_ || textInputActive_) CancelPendingAnnotation();
                        else running = false;
                    } else {
                        HandleKey(ev.key.keysym.sym, ev.key.keysym.mod);
                    }
                    break;
                case SDL_TEXTINPUT:
                    HandleTextInput(ev.text.text);
                    break;
                case SDL_MOUSEMOTION: HandleMouseMotion(ev.motion.x, ev.motion.y); break;
                case SDL_MOUSEBUTTONDOWN: HandleMouseButton(ev.button.button, true, ev.button.x, ev.button.y); break;
                case SDL_MOUSEBUTTONUP:   HandleMouseButton(ev.button.button, false, ev.button.x, ev.button.y); break;
                case SDL_MOUSEWHEEL:      HandleMouseWheel(ev.wheel.y); break;
                default: break;
            }
        }
        RenderFrame();
        UpdateWindowTitle();
        SDL_Delay(16);
    }

    CleanupSDL();
    return 0;
}

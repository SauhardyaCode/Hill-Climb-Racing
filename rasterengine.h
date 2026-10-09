#ifndef RASTERENGINE_H
#define RASTERENGINE_H

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <QImage>
#include <QColor>
#include <QPoint>

// High performance 32-bit ARGB color packing
inline uint32_t makeColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return (static_cast<uint32_t>(a) << 24) |
           (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8)  |
           (static_cast<uint32_t>(b));
}

inline uint8_t colorR(uint32_t c) { return (c >> 16) & 0xFF; }
inline uint8_t colorG(uint32_t c) { return (c >> 8) & 0xFF; }
inline uint8_t colorB(uint32_t c) { return c & 0xFF; }
inline uint8_t colorA(uint32_t c) { return (c >> 24) & 0xFF; }

// Alpha blending of fg over bg
inline uint32_t blendColor(uint32_t bg, uint32_t fg) {
    uint32_t a = colorA(fg);
    if (a == 255) return fg;
    if (a == 0) return bg;

    uint32_t invA = 255 - a;
    uint32_t r = (colorR(fg) * a + colorR(bg) * invA) / 255;
    uint32_t g = (colorG(fg) * a + colorG(bg) * invA) / 255;
    uint32_t b = (colorB(fg) * a + colorB(bg) * invA) / 255;
    return makeColor(r, g, b, 255);
}

class RasterBuffer {
public:
    RasterBuffer(int width, int height);
    ~RasterBuffer();

    void resize(int width, int height);
    void clear(uint32_t color);

    int width() const { return m_width; }
    int height() const { return m_height; }
    uint32_t* data() { return m_pixels.data(); }
    const uint32_t* data() const { return m_pixels.data(); }

    inline void setPixel(int x, int y, uint32_t color) {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            if (colorA(color) == 255) {
                m_pixels[y * m_width + x] = color;
            } else {
                m_pixels[y * m_width + x] = blendColor(m_pixels[y * m_width + x], color);
            }
        }
    }

    inline uint32_t getPixel(int x, int y) const {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            return m_pixels[y * m_width + x];
        }
        return 0;
    }

    // Direct pixel setting without bounds checking (when caller verified)
    inline void setPixelUnsafe(int x, int y, uint32_t color) {
        m_pixels[y * m_width + x] = color;
    }

    // Core computer graphics rasterization primitives
    void drawLineBresenham(int x0, int y0, int x1, int y1, uint32_t color, int thickness = 1);
    void drawCircleMidpoint(int xc, int yc, int r, uint32_t color);
    void drawCircleFilled(int xc, int yc, int r, uint32_t color);
    void drawCircleThick(int xc, int yc, int r, int thickness, uint32_t color);
    void drawRectFilled(int x, int y, int w, int h, uint32_t color);
    void drawRectOutline(int x, int y, int w, int h, uint32_t color);
    void drawVerticalSpan(int x, int yTop, int yBottom, uint32_t color);
    void drawHorizontalSpan(int y, int xLeft, int xRight, uint32_t color);
    void drawTriangleFilled(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
    void drawPolygonFilled(const std::vector<QPoint>& vertices, uint32_t color);

    // Rasterized bitmap font rendering
    void drawBitmapChar(char c, int x, int y, uint32_t color, int scale = 1);
    void drawBitmapString(const std::string& text, int x, int y, uint32_t color, int scale = 1);
    int getTextWidth(const std::string& text, int scale = 1) const;

    // Convert buffer to QImage for Qt display, with optional grid overlay
    QImage toQImage(bool showGrid = false, int gridCellSize = 1, uint32_t gridLineColor = 0xFF303030) const;

private:
    int m_width;
    int m_height;
    std::vector<uint32_t> m_pixels;
};

#endif // RASTERENGINE_H

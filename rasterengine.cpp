#include "rasterengine.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// 5x7 ASCII bitmap font for characters 32 to 126
// Each char is represented by 5 columns of 7 bits
static const uint8_t FONT_5X7[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // ' ' (32)
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // '!'
    {0x00, 0x07, 0x00, 0x07, 0x00}, // '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // '$'
    {0x23, 0x13, 0x08, 0x64, 0x62}, // '%'
    {0x36, 0x49, 0x55, 0x22, 0x50}, // '&'
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '''
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // '('
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // '+'
    {0x00, 0x50, 0x30, 0x00, 0x00}, // ','
    {0x08, 0x08, 0x08, 0x08, 0x08}, // '-'
    {0x00, 0x60, 0x60, 0x00, 0x00}, // '.'
    {0x20, 0x10, 0x08, 0x04, 0x02}, // '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // '1'
    {0x42, 0x61, 0x51, 0x49, 0x46}, // '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // '4'
    {0x27, 0x45, 0x45, 0x45, 0x39}, // '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // '6'
    {0x01, 0x71, 0x09, 0x05, 0x03}, // '7'
    {0x36, 0x49, 0x49, 0x49, 0x36}, // '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // '9'
    {0x00, 0x36, 0x36, 0x00, 0x00}, // ':'
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ';'
    {0x08, 0x14, 0x22, 0x41, 0x00}, // '<'
    {0x14, 0x14, 0x14, 0x14, 0x14}, // '='
    {0x00, 0x41, 0x22, 0x14, 0x08}, // '>'
    {0x02, 0x01, 0x51, 0x09, 0x06}, // '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07}, // 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // '['
    {0x02, 0x04, 0x08, 0x10, 0x20}, // '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // ']'
    {0x04, 0x02, 0x01, 0x02, 0x04}, // '^'
    {0x40, 0x40, 0x40, 0x40, 0x40}, // '_'
    {0x00, 0x01, 0x02, 0x04, 0x00}, // '`'
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E}, // 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00}, // '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // '|'
    {0x00, 0x41, 0x36, 0x08, 0x00}, // '}'
    {0x08, 0x08, 0x2A, 0x1C, 0x08}  // '~'
};

RasterBuffer::RasterBuffer(int width, int height)
    : m_width(width)
    , m_height(height)
    , m_pixels(width * height, 0xFF000000)
{
}

RasterBuffer::~RasterBuffer() = default;

void RasterBuffer::resize(int width, int height) {
    if (m_width == width && m_height == height) return;
    m_width = width;
    m_height = height;
    m_pixels.assign(width * height, 0xFF000000);
}

void RasterBuffer::clear(uint32_t color) {
    std::fill(m_pixels.begin(), m_pixels.end(), color);
}

// Bresenham's Line Algorithm
void RasterBuffer::drawLineBresenham(int x0, int y0, int x1, int y1, uint32_t color, int thickness) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        if (thickness <= 1) {
            setPixel(x0, y0, color);
        } else {
            int half = thickness / 2;
            for (int ty = -half; ty <= half; ++ty) {
                for (int tx = -half; tx <= half; ++tx) {
                    setPixel(x0 + tx, y0 + ty, color);
                }
            }
        }

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

// Midpoint Circle Algorithm (8-way symmetry outline)
void RasterBuffer::drawCircleMidpoint(int xc, int yc, int r, uint32_t color) {
    if (r < 0) return;
    int x = 0;
    int y = r;
    int d = 1 - r;

    auto plot8 = [this, xc, yc, color](int x, int y) {
        setPixel(xc + x, yc + y, color);
        setPixel(xc - x, yc + y, color);
        setPixel(xc + x, yc - y, color);
        setPixel(xc - x, yc - y, color);
        setPixel(xc + y, yc + x, color);
        setPixel(xc - y, yc + x, color);
        setPixel(xc + y, yc - x, color);
        setPixel(xc - y, yc - x, color);
    };

    plot8(x, y);
    while (x < y) {
        ++x;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            --y;
            d += 2 * (x - y) + 1;
        }
        plot8(x, y);
    }
}

// Filled Circle (Scanline Midpoint Circle)
void RasterBuffer::drawCircleFilled(int xc, int yc, int r, uint32_t color) {
    if (r < 0) return;
    for (int y = -r; y <= r; ++y) {
        int py = yc + y;
        if (py < 0 || py >= m_height) continue;
        int halfW = static_cast<int>(std::sqrt(r * r - y * y));
        int xStart = std::max(0, xc - halfW);
        int xEnd = std::min(m_width - 1, xc + halfW);
        drawHorizontalSpan(py, xStart, xEnd, color);
    }
}

// Thick Circle / Tire Rim
void RasterBuffer::drawCircleThick(int xc, int yc, int r, int thickness, uint32_t color) {
    int rMin = std::max(0, r - thickness / 2);
    int rMax = r + thickness / 2;
    for (int curR = rMin; curR <= rMax; ++curR) {
        drawCircleMidpoint(xc, yc, curR, color);
    }
}

void RasterBuffer::drawRectFilled(int x, int y, int w, int h, uint32_t color) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(m_width - 1, x + w - 1);
    int y1 = std::min(m_height - 1, y + h - 1);

    for (int cy = y0; cy <= y1; ++cy) {
        drawHorizontalSpan(cy, x0, x1, color);
    }
}

void RasterBuffer::drawRectOutline(int x, int y, int w, int h, uint32_t color) {
    drawLineBresenham(x, y, x + w - 1, y, color);
    drawLineBresenham(x, y + h - 1, x + w - 1, y + h - 1, color);
    drawLineBresenham(x, y, x, y + h - 1, color);
    drawLineBresenham(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void RasterBuffer::drawVerticalSpan(int x, int yTop, int yBottom, uint32_t color) {
    if (x < 0 || x >= m_width) return;
    int y0 = std::max(0, std::min(yTop, yBottom));
    int y1 = std::min(m_height - 1, std::max(yTop, yBottom));

    uint32_t a = colorA(color);
    if (a == 255) {
        for (int y = y0; y <= y1; ++y) {
            m_pixels[y * m_width + x] = color;
        }
    } else {
        for (int y = y0; y <= y1; ++y) {
            m_pixels[y * m_width + x] = blendColor(m_pixels[y * m_width + x], color);
        }
    }
}

void RasterBuffer::drawHorizontalSpan(int y, int xLeft, int xRight, uint32_t color) {
    if (y < 0 || y >= m_height) return;
    int x0 = std::max(0, std::min(xLeft, xRight));
    int x1 = std::min(m_width - 1, std::max(xLeft, xRight));

    uint32_t a = colorA(color);
    if (a == 255) {
        uint32_t* row = &m_pixels[y * m_width + x0];
        int count = x1 - x0 + 1;
        std::fill(row, row + count, color);
    } else {
        for (int x = x0; x <= x1; ++x) {
            m_pixels[y * m_width + x] = blendColor(m_pixels[y * m_width + x], color);
        }
    }
}

// Scanline Triangle Rasterization
void RasterBuffer::drawTriangleFilled(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    // Sort vertices by Y (y0 <= y1 <= y2)
    if (y0 > y1) { std::swap(x0, x1); std::swap(y0, y1); }
    if (y0 > y2) { std::swap(x0, x2); std::swap(y0, y2); }
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }

    int totalHeight = y2 - y0;
    if (totalHeight == 0) return;

    for (int y = y0; y <= y2; ++y) {
        bool secondHalf = (y > y1) || (y1 == y0);
        int segmentHeight = secondHalf ? (y2 - y1) : (y1 - y0);
        if (segmentHeight == 0) continue;

        float alpha = static_cast<float>(y - y0) / totalHeight;
        float beta = static_cast<float>(y - (secondHalf ? y1 : y0)) / segmentHeight;

        int ax = static_cast<int>(x0 + (x2 - x0) * alpha);
        int bx = secondHalf ? static_cast<int>(x1 + (x2 - x1) * beta)
                            : static_cast<int>(x0 + (x1 - x0) * beta);

        if (ax > bx) std::swap(ax, bx);
        drawHorizontalSpan(y, ax, bx, color);
    }
}

// Scanline Polygon Fill Algorithm
void RasterBuffer::drawPolygonFilled(const std::vector<QPoint>& vertices, uint32_t color) {
    if (vertices.size() < 3) return;

    int minY = m_height, maxY = 0;
    for (const auto& v : vertices) {
        minY = std::min(minY, v.y());
        maxY = std::max(maxY, v.y());
    }
    minY = std::max(0, minY);
    maxY = std::min(m_height - 1, maxY);

    std::vector<int> nodeX;
    size_t n = vertices.size();

    for (int y = minY; y <= maxY; ++y) {
        nodeX.clear();
        size_t j = n - 1;
        for (size_t i = 0; i < n; ++i) {
            int y_i = vertices[i].y();
            int y_j = vertices[j].y();
            int x_i = vertices[i].x();
            int x_j = vertices[j].x();

            if ((y_i < y && y_j >= y) || (y_j < y && y_i >= y)) {
                int x = x_i + static_cast<int>(static_cast<double>(y - y_i) / (y_j - y_i) * (x_j - x_i));
                nodeX.push_back(x);
            }
            j = i;
        }

        std::sort(nodeX.begin(), nodeX.end());
        for (size_t k = 0; k + 1 < nodeX.size(); k += 2) {
            drawHorizontalSpan(y, nodeX[k], nodeX[k + 1], color);
        }
    }
}

// Rasterized Bitmap Font Rendering
void RasterBuffer::drawBitmapChar(char c, int x, int y, uint32_t color, int scale) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t* charData = FONT_5X7[c - 32];

    for (int col = 0; col < 5; ++col) {
        uint8_t line = charData[col];
        for (int row = 0; row < 7; ++row) {
            if (line & (1 << row)) {
                if (scale == 1) {
                    setPixel(x + col, y + row, color);
                } else {
                    for (int dy = 0; dy < scale; ++dy) {
                        for (int dx = 0; dx < scale; ++dx) {
                            setPixel(x + col * scale + dx, y + row * scale + dy, color);
                        }
                    }
                }
            }
        }
    }
}

void RasterBuffer::drawBitmapString(const std::string& text, int x, int y, uint32_t color, int scale) {
    int curX = x;
    int charSpacing = 6 * scale;
    for (char c : text) {
        if (c == '\n') {
            y += 8 * scale;
            curX = x;
            continue;
        }
        drawBitmapChar(c, curX, y, color, scale);
        curX += charSpacing;
    }
}

int RasterBuffer::getTextWidth(const std::string& text, int scale) const {
    int maxLen = 0;
    int curLen = 0;
    for (char c : text) {
        if (c == '\n') {
            maxLen = std::max(maxLen, curLen);
            curLen = 0;
        } else {
            ++curLen;
        }
    }
    maxLen = std::max(maxLen, curLen);
    return maxLen * 6 * scale;
}

// Convert to QImage with optional grid lines overlay
QImage RasterBuffer::toQImage(bool showGrid, int gridCellSize, uint32_t gridLineColor) const {
    if (!showGrid || gridCellSize <= 1) {
        // Direct pixel buffer copy to QImage
        QImage img(reinterpret_cast<const uchar*>(m_pixels.data()),
                   m_width, m_height, m_width * sizeof(uint32_t),
                   QImage::Format_ARGB32_Premultiplied);
        return img.copy();
    }

    // Grid rendering mode: upscale pixels to cell size and draw grid cell borders
    int outW = m_width * gridCellSize;
    int outH = m_height * gridCellSize;
    QImage gridImg(outW, outH, QImage::Format_ARGB32_Premultiplied);
    uint32_t* outPixels = reinterpret_cast<uint32_t*>(gridImg.bits());

    for (int gy = 0; gy < m_height; ++gy) {
        for (int gx = 0; gx < m_width; ++gx) {
            uint32_t cellColor = m_pixels[gy * m_width + gx];
            int pxStart = gx * gridCellSize;
            int pyStart = gy * gridCellSize;

            for (int dy = 0; dy < gridCellSize; ++dy) {
                int py = pyStart + dy;
                for (int dx = 0; dx < gridCellSize; ++dx) {
                    int px = pxStart + dx;
                    if (dx == 0 || dy == 0) {
                        outPixels[py * outW + px] = gridLineColor;
                    } else {
                        outPixels[py * outW + px] = cellColor;
                    }
                }
            }
        }
    }

    return gridImg;
}

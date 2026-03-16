# Assignment 1 -- Image Processing

CPU-based image processing pipeline built on top of an OpenGL engine. The program loads a source image (`Lenna.png`) and applies four classical image processing algorithms, producing both a `.png` image and a `.txt` data file for each.

All processing is done on raw pixel buffers on the CPU. No OpenGL or engine code was modified.

## Results

<p align="center">
  <img src="bin/res/textures/Lenna.png" width="150" alt="Original" />
  <img src="bin/res/textures/Grayscale.png" width="150" alt="Grayscale" />
  <img src="bin/res/textures/Canny.png" width="150" alt="Canny Edge Detection" />
  <img src="bin/res/textures/Halftone.png" width="150" alt="Halftone" />
  <img src="bin/res/textures/FloyedSteinberg.png" width="150" alt="Floyd-Steinberg" />
</p>
<p align="center">
  <em>Original &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Grayscale &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Canny &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Halftone &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Floyd-Steinberg</em>
</p>

## Grayscale Conversion

Converts an RGBA image to a single-channel grayscale image using the **channel averaging** method. Each pixel's intensity is computed as the mean of its R, G, and B values. The output `.txt` file contains the quantized intensity of every pixel mapped to the range 0--15.

## Canny Edge Detection

A multi-stage edge detection pipeline:

1. **Gaussian Blur** -- Smooths the image with a 5x5 Gaussian kernel to reduce noise.
2. **Sobel Operator** -- Computes horizontal and vertical gradient magnitudes using 3x3 Sobel convolution kernels.
3. **Non-Maximum Suppression** -- Thins edges by keeping only pixels that are local maxima along the gradient direction (quantized to 0, 45, 90, 135 degrees).
4. **Double Thresholding** -- Classifies pixels into strong edges, weak edges, or non-edges using a low threshold (30) and a high threshold (80).
5. **Hysteresis** -- Performs BFS from all strong edge pixels to promote connected weak edges to strong, then discards any remaining weak edges.

The output `.txt` file contains binary values (0 or 1) for each pixel.

## Halftone

Simulates a printed halftone effect by replacing each grayscale pixel with a **2x2 dot pattern**. The pixel intensity is quantized into 5 levels (0--4), determining how many of the four sub-pixels are filled black. This produces an output image that is twice the width and height of the original. The output `.txt` file contains binary values (0 or 1) for each sub-pixel.

## Floyd-Steinberg Dithering

Reduces the grayscale image to **16 discrete levels** using error-diffusion dithering. For each pixel, the algorithm quantizes its value to the nearest of 16 levels, then distributes the quantization error to neighboring pixels using the Floyd-Steinberg weights:

- 7/16 to the right
- 3/16 to the bottom-left
- 5/16 to the bottom
- 1/16 to the bottom-right

This creates the visual illusion of more tonal depth than the 16 levels actually present. The output `.txt` file contains the quantized level (0--15) of every pixel.

## Build and Run

```bash
make
cd bin
./main
```

The executable loads `res/textures/Lenna.png` and writes all output images and text files into the working directory.

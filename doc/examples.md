Examples
========

Intel Open Image Denoise ships with a couple of simple example applications.

oidnDenoise
-----------

`oidnDenoise` is a minimal working example demonstrating how to use Intel Open
Image Denoise, which can be found at `apps/oidnDenoise.cpp`. It uses the C++11
convenience wrappers of the C99 API.

This example is a simple command-line application that denoises the provided
image, which can optionally have auxiliary feature images as well (e.g. albedo
and normal). By default the images must be stored in the [Portable
FloatMap](http://www.pauldebevec.com/Research/HDR/PFM/) (PFM) format, and the
color values must be encoded in little-endian format. To enable other image
formats (e.g. OpenEXR, PNG) as well, the project has to be rebuilt with
OpenImageIO support enabled.

When denoising an image sequence, temporal accumulation can be enabled by
additionally passing the denoised output of the previous frame (`--prev`) and
the motion vectors of the current frame (`--motion`), e.g.:

    oidnDenoise --hdr color_0042.pfm --alb albedo_0042.pfm --nrm normal_0042.pfm \
                --prev denoised_0041.pfm --motion motion_0042.pfm \
                --prev_alb albedo_0041.pfm --prev_nrm normal_0041.pfm \
                -o denoised_0042.pfm

The maximum weight of the previous frame can be controlled with the
`--temporal_blend` option (default: 0.8).

Running `oidnDenoise` without any arguments or the `-h` argument will bring up
a list of command-line options.

oidnBenchmark
-------------

`oidnBenchmark` is a basic command-line benchmarking application for measuring
denoising speed, which can be found at `apps/oidnBenchmark.cpp`.

Running `oidnBenchmark` with the `-h` argument will bring up a list of
command-line options.

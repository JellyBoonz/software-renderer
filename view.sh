#!/bin/bash
magick framebuffer.ppm -scale 800% framebuffer.png
open framebuffer.png

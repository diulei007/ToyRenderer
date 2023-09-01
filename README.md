# ToyRenderer

ToyRenderer是一个简单的渲染器项目，使用C++实现了：

    1.whitted风格的基础光线追踪（basicRT）

    2.BVH构建与SAH优化光线跟踪(BVH)

    3.基于opengl环境的光线追踪（openglRT）

    4.disney风格的BRDF(Disney Principle BRDF）

    5.基于低差异序列与重要性采样的光线优化(LDS_ISS）

<div align="center"><img src="README.assets/image-20211023163925119.png" width="300"></div>

## part 1. whitted风格的基础光线追踪

    在这一部分中，使用C++与一个轻量的图形生成库svpng实现一个基本的光线跟踪程序，通过逐像素颜色计算并将每个像素作为结果输出到png图像中。

<div align="center"><img src="README.assets/image-20211023164946107.png" width="256"></div>

## part 2. 层次包围盒BVH与其优化方法

    在光线追踪算法中用BVH方法来做加速。同时使用SAH计算cost优化BVH。

<div align="center"><img src="README.assets/081d0df0237c4a9aa27f3a79b6ea1ecd.png" width="256"></div>

## part 3. OpenGL ray tracing

    使用OpenGL的片段着色器运行加速程序。在纹理缓冲区中传输BVH和三角形，然后发送到着色器。最后逐步跟踪每个像素，然后在屏幕上显示动态跟踪过程。

<div align="center"><img src="README.assets/28131dad2ea74fd2ad11125e69db62f1-1634979630086.gif" width="256"></div>

## part 4. disney principle's BRDF

包括微平面理论，运用迪士尼原理的BRDF绘制各种物理材料。

![image-20211023170631173](README.assets/image-20211023170631173.png)

## part 5. Importance Sampling & Low Discrepancy Sequence

去噪，加快拟合进度。

低差异序列（Sobol）：

<div align="center"><img src="README.assets/2eb9495dd5024b4f914dc3291a0467ba.gif" width="256"> <img src="README.assets/image-20211023171623698.png" width="256"></div>

重要性采样（Importance Sampling）、diffuse（左）和BRDF（右）：

<div align="center"><img src="README.assets/800c3511ae8043c08e3e43cb1e7ef8f6.png" width="512"></div>

HDR环境映射的重要性采样：

<div align="center"><img src="README.assets/image-20211023172136160.png" width="512"></div>

多重重要性采样：

<div align="center"><img src="README.assets/29a21706e2664d57a2ca9a6089da632a.png" width="300"></div>

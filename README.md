<h1>Image Resampling using Mitchell-Netravali filter</h1>

<p>I've been spending a bit of time reading about image signal processing and decided that putting what I have learnt to use is a reasonable idea.</p>

A few notes about the implementation:
<p><ul style="padding-left:20px">
  <li>Separable filters.</li>
  <li>Two-pass horizontal followed by vertical resampling, array stores intermediate results to avoid redundant computations which saves clock cycles.</li>
  <li>Sets the radius of our convolution depending on whether we are downsampling or upsampling.</li>
  <li>Uses OpenMP to spread horizontal and vertical pass across many threads. Around ~30% uplift since image-resampling has low arithmetic intensity (memory bandwidth bound).</li>
</ul>

When upsampling, we can afford to use a smaller filter radius because the alias spectra and base spectrum in frequency domain should be far enough apart that they don't mix and introduce artifacts. 

In contrast, when downsampling, it is likely that the distance between the alias spectra and base spectrum is not sufficient and they mix. This results in artifacts (aliasing) in high frequency areas like edges. To avoid this, we can remove the high frequencies that are likely to mix by increasing the filter radius. The result is a smoother (blurrier) image but reduced aliasing.

Below is an example of downsampling, shrinking an image to 1/3rd of its original size. The image on the left uses a quick and dirty pixel dropping approach and on the right uses this resampler. Feel free to click on them to get a better look.



Pixel Dropping           |  Mitchell-Netravali
:-------------------------:|:-------------------------:
![red_panda_pixel_drop](https://github.com/user-attachments/assets/89fb5e24-f34c-4066-b1c9-5e64490e14c1)  |   ![red_panda_cubic](https://github.com/user-attachments/assets/d80536ce-2530-4213-9739-4419e81136c5)





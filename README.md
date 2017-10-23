Scan Tailor Advanced
========================

The Scan Tailor version that merges the features of the `Scan Tailor Featured` and `Scan Tailor Enhanced` versions,
brings new ones and fixes.


#### <u>Contents</u>:
* [Description](#description)
* [Features](#features)
    * [**Scan Tailor Enhanced** features](#scan-tailor-enhanced-features)
        * [Auto margins](#auto-margins)
        * [Page detect](#page-detect)
        * [Deviation](#deviation)
        * [Picture shape](#picture-shape)
        * [Tiff compression](#tiff-compression)
        * [Multi column thumbnails view \[reworked\]](#multi-column-thumbnails-view-reworked)
    * [**Scan Tailor Featured** features](#scan-tailor-featured-features)
        * [Scan Tailor Featured fixes](#scan-tailor-featured-fixes)
        * [Line vertical dragging on dewarp](#line-vertical-dragging-on-dewarp)
        * [Square picture zones](#square-picture-zones)
        * [Auto save project \[optimized\]](#auto-save-project-optimized)
        * [Quadro Zoner](#quadro-zoner)
        * [Marginal dewarping](#marginal-dewarping)
    * [**Scan Tailor Advanced** features](#scan-tailor-advanced-features)
        * [Scan Tailor Advanced fixes](#scan-tailor-advanced-fixes)
        * [Light and Dark color schemes](#light-and-dark-color-schemes)
        * [Multi-threading support for batch processing](#multi-threading-support-for-batch-processing)
        * [Full control over settings on output](#full-control-over-settings-on-output)
        * [Adaptive binarization](#adaptive-binarization)
        * [Splitting output](#splitting-output)
* [Building](#building)

Description
------------

Scan Tailor is an interactive post-processing tool for scanned pages. 
It performs operations such as:
  - [page splitting](https://github.com/scantailor/scantailor/wiki/Split-Pages), 
  - [deskewing](https://github.com/scantailor/scantailor/wiki/Deskew), 
  - [adding/removing borders](https://github.com/scantailor/scantailor/wiki/Page-Layout), 
  - [selecting content](https://github.com/scantailor/scantailor/wiki/Select-Content) 
  - ... and others. 
  
You give it raw scans, and you get pages ready to be printed or assembled into a PDF 
  or [DjVu](http://elpa.gnu.org/packages/djvu.html) file. Scanning, optical character recognition, 
  and assembling multi-page documents are out of scope of this project.

Features
----------

#### **Scan Tailor Enhanced** features

* ##### Auto margins
 Auto margins feature allows keep page content on original place. In the Margins step
 you can choose from Auto, Manual (default) and Original mode. The manual mode
 is the original one. Auto mode try to decide if it is better to align page top,
 bottom or center. Original mode keeps page on their vertical original position.

* ##### Page detect
 Page detect feature allows detect page in black margins or switch off page content
 detection and keep original page layout. 
 
* ##### Deviation
 Deviation feature enables highlighting of different pages. Highlighted in red are pages
 from Deskew filter with too high skew, from Select Content filter pages with different
 size of content and in Margins filter are highlighted pages which does not match others.
 
* ##### Picture shape
 Picture shape feature adds option for mixed pages to choose from free shape and rectangular
 shape images. This patch does not improve the original algoritm but creates from the
 detected "blobs" rectangular shapes and the rectangles that intersects joins to one.
 
* ##### Tiff compression
 Tiff compression option allows to disable compression in tiff files or select one of other
 standard compression methods (LZW, Deflate, PackBits, Jpeg).

* ##### Multi column thumbnails view \[reworked\]
 This allows to expand and un-dock thumbnails view to see more thumbnails at a time.

 *This feature had performance and drawing issues and has been reworked.*
 
#### **Scan Tailor Featured** features

* ##### Scan Tailor Featured fixes
1. Deleted 3 Red Points 
 The 3 central red points on the topmost (bottom-most) horizontal blue line of the dewarping
 mesh are now eliminated. 
2. Manual dewarping mode auto switch 
 The dewarping mode is now set to MANUAL (from OFF) after the user has moved the dewarping mesh.
3. Auto dewarping vertical half correction 
 This patch corrects the original auto-dewarping in half
 the cases when it fails. If the vertical content boundary angle (calculated by auto-dewarping)
 exceeds an empirical value (2.75 degrees from vertical), the patch adds a new point to
 the distortion model (with the coordinates equal to the neighboring points) to make
 this boundary vertical. The patch works ONLY for the linear end of the top (bottom)
 horizontal line of the blue mesh (and not for the opposite curved end).
 
* ##### Line vertical dragging on dewarp
 You can move the topmost (bottom-most) horizontal blue line of the dewarping mesh up and
 down as a whole - if you grab it at the most left (right) red point - holding down the CTRL key. 
 
* ##### Square picture zones
 You can create the rectangular picture zones - holding down the CTRL key. 
 You can move the (rectangular) picture zones corners in an orthogonal manner - holding down the CTRL key.
  
* ##### Auto save project \[optimized\]
 Set the "auto-save project" checked in the Settings menu and you will get 
 your project auto-saved provided you have originally saved your new project.
 Works at the batch processing too. 
 
 *This feature had performance issues and has been optimized.*
 
* ##### Quadro Zoner
 Another rectangular picture zone shape. This option is based on [Picture shape](#picture-shape),
 [Square picture zones](#square-picture-zones). It squeezes every Picture shape zone down to the real
 rectangular picture outline and then replaces it (the resulting raster zone) by a vector rectangular zone,
 so that a user could easily adjust it afterwards (by moving its corners in an orthogonal manner).
 
* ##### Marginal dewarping 
 An automatic dewarping mode. Works ONLY with such raw scans that have the top and 
 bottom curved page borders (on the black background). It automatically sets the red points 
 of the blue mesh along these borders (to create a distortion model) and then dewarps the scan 
 according to them. Works best on the low-curved scans. 
 
 
\**Other features of this version, such as Export, Dont_Equalize_Illumination_Pic_Zones, Original_Foreground_Mixed
   has't been moved due to dirty realization. Their functionality is fully covered by 
   [Full control over settings on output](#full-control-over-settings-on-output) and 
   [Splitting output](#splitting-output) features.*

#### **Scan Tailor Advanced** features

* ##### Scan Tailor Advanced fixes
1. Portability.
   The setting is stored in the folder with a program.

2. Page splitting had an influence on output only in b&w mode with dewarping disabled.
   Now it works in all the modes.

3. Dewarping in manual mode with binarization enabled produced low-quality results due to post deskew.
   Now deskew is done before binarization.

4. Optimized memory usage on the output stage.

5. Reworking on [Multi column thumbnails view](#multi-column-thumbnails-view-reworked)
   feature from ver. Enhanced. 
   Now thumbnails is shown evenly.

* ##### Light and Dark color schemes
 You can choose a desired color scheme in settings.

* ##### Multi-threading support for batch processing
 This significantly increases the speed of processing. The count of threads to use can be
 adjusted while processing.

 **Warning!** More threads requires more memory to use. Exclude situations of that to be overflowed.  

* ##### Full control over settings on output
 This feature enables to control white margins, normalizing illumination before binarization,
 normalizing illumination in color areas options, Savitzky-Golay and morphological smoothing on output
 in any mode (of course, those setting that can be applied in the current mode).  
 
* ##### Adaptive binarization
 Sauvola and Wolf binarization algorithms have been added. They can be applied when
 normalizing illumination does not help.
 
* ##### Splitting output
 The feature allows to split the mixed output scans into the pairs of a foreground (letters) 
 and background (images) layer.
 
 You can choose between B&W or color (original) foreground.
 
 It can be useful:
   -    for the further DjVu encoding,
   -    to apply different filters to letters and images, which when being applied to the whole
        image gives worse results.
   -    to apply a binarization to the letters from a third party app without affecting the images.
   
 *Note: That does not rename files to 0001, 0002... It can be made by a third party app, for example 
 [Bulk Rename Utility](http://www.bulkrenameutility.co.uk/Main_Intro.php)*

Building
----------

Go to [this repository](https://github.com/4lex4/scantailor-libs-build) and follow the instructions given there.
ScanTailor Advanced
===================

The ScanTailor version that merges the features of the `ScanTailor Featured` and `ScanTailor Enhanced` versions,
brings new ones and fixes.  


#### <u>Contents</u>:
* [Description](#description)
* [Features](#features)
    * [**ScanTailor Enhanced**](#scantailor-enhanced)
        * [Auto margins \[improved\]](#auto-margins-improved)
        * [Page detect \[reworked\]](#page-detect-reworked)
        * [Deviation \[reworked\]](#deviation-reworked)
        * [Picture shape \[reworked\]](#picture-shape-reworked)
        * [Multi column thumbnails view \[reworked\]](#multi-column-thumbnails-view-reworked)
    * [**ScanTailor Featured**](#scantailor-featured)
        * [ScanTailor Featured fixes & improvements](#scantailor-featured-fixes--improvements)
        * [Line vertical dragging on dewarp](#line-vertical-dragging-on-dewarp)
        * [Square picture zones \[reworked\]](#square-picture-zones-reworked)
        * [Auto save project \[optimized\]](#auto-save-project-optimized)
        * [Quadro Zoner \[reworked\]](#quadro-zoner-reworked)
        * [Marginal dewarping](#marginal-dewarping)
    * [**ScanTailor Advanced**](#scantailor-advanced-features)
        * [ScanTailor Advanced fixes & improvements](#scantailor-advanced-fixes--improvements)
        * [Light and Dark color schemes](#light-and-dark-color-schemes)
        * [Multi-threading support for batch processing](#multi-threading-support-for-batch-processing)
        * [Full control over settings on output](#full-control-over-settings-on-output)
        * [Filling outside areas](#filling-outside-areas)
        * [Tiff compression](#tiff-compression)
        * [Adaptive binarization](#adaptive-binarization)
        * [Splitting output](#splitting-output)
        * [Original background](#original-background)
        * [Color segmenter and posterization](#color-segmenter-and-posterization)
        * [Rectangular picture shape](#rectangular-picture-shape)
        * [Page area](#page-area)
        * [New zone interaction modes](#new-zone-interaction-modes)
        * [Saving zoom and focus on switching output tabs](#saving-zoom-and-focus-on-switching-output-tabs)
        * [Measurement units system](#measurement-units-system)
        * [Status bar panel](#status-bar-panel)
        * [Default parameters](#default-parameters)
        * [Collapsible filter options](#collapsible-filter-options)
        * [Auto adjusting content area](#auto-adjusting-content-area)
        * [Black on white detection](#black-on-white-detection)
        * [Guides](#guides)
* [Building](#building)

Description
------------

ScanTailor is an interactive post-processing tool for scanned pages. 
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

#### ScanTailor Enhanced

* ##### Auto margins \[improved\]
  Auto margins feature allows keep page content on original place. In the Margins step
  you can choose from Auto, Manual (default) and Original mode. The manual mode
  is the original one. Auto mode try to decide if it is better to align page top,
  bottom or center. Original mode keeps page on their vertical original position.
  
  *This feature has been improved. See [page area](#page-area) feature description.*
  *Also see [ScanTailor Advanced fixes & improvements](#scantailor-advanced-fixes--improvements)*

* ##### Page detect \[reworked\]
  Page detect feature allows detect page in black margins or switch off page content
  detection and keep original page layout.
  
  *This feature has been reworked and is now a part of the [page area](#page-area) feature.*
 
* ##### Deviation \[reworked\]
  Deviation feature enables highlighting of different pages. Highlighted in red are pages
  from Deskew filter with too high skew, from Select Content filter pages with different
  size of content and in Margins filter are highlighted pages which does not match others.
  
  *This feature has been reworked. See [ScanTailor Advanced fixes & improvements](#scantailor-advanced-fixes--improvements) for more information.*
 
* ##### Picture shape \[reworked\]
  Picture shape feature adds option for mixed pages to choose from free shape and rectangular
  shape images. This patch does not improve the original algoritm but creates from the
  detected "blobs" rectangular shapes and the rectangles that intersects joins to one.
  
  *This feature has been reworked. See [rectangular picture shape](#rectangular-picture-shape) feature description.*

* ##### Multi column thumbnails view \[reworked\]
  This allows to expand and un-dock thumbnails view to see more thumbnails at a time.
  
  *This feature had performance and drawing issues and has been reworked.*
 
#### ScanTailor Featured

* ##### ScanTailor Featured fixes & improvements
    * Deleted 3 Red Points.   
      The 3 central red points on the topmost (bottom-most) horizontal blue line of the dewarping
      mesh are now eliminated. 
    * Manual dewarping mode auto switch.   
      The dewarping mode is now set to MANUAL (from OFF) after the user has moved the dewarping mesh.
    * Auto dewarping vertical half correction.   
      This patch corrects the original auto-dewarping in half
      the cases when it fails. If the vertical content boundary angle (calculated by auto-dewarping)
      exceeds an empirical value (2.75 degrees from vertical), the patch adds a new point to
      the distortion model (with the coordinates equal to the neighboring points) to make
      this boundary vertical. The patch works ONLY for the linear end of the top (bottom)
      horizontal line of the blue mesh (and not for the opposite curved end).
 
* ##### Line vertical dragging on dewarp
  You can move the topmost (bottom-most) horizontal blue line of the dewarping mesh up and
  down as a whole - if you grab it at the most left (right) red point - holding down the CTRL key. 
 
* ##### Square picture zones \[reworked\]
  You can create the rectangular picture zones - holding down the CTRL key. 
  You can move the (rectangular) picture zones corners in an orthogonal manner - holding down the CTRL key.
  
  *This feature has been reworked and is now a part of [new zone interaction modes](#new-zone-interaction-modes) feature.*
  
* ##### Auto save project \[optimized\]
  Set the "auto-save project" checked in the Settings menu and you will get 
  your project auto-saved provided you have originally saved your new project.
  Works at the batch processing too. 
  
  *This feature had performance issues and has been optimized.*
 
* ##### Quadro Zoner \[reworked\]
  Another rectangular picture zone shape. This option is based on [Picture shape](#picture-shape),
  [Square picture zones](#square-picture-zones). It squeezes every Picture shape zone down to the real
  rectangular picture outline and then replaces it (the resulting raster zone) by a vector rectangular zone,
  so that a user could easily adjust it afterwards (by moving its corners in an orthogonal manner).
  
  *This feature has been reworked. See [rectangular picture shape](#rectangular-picture-shape) feature description.*
 
* ##### Marginal dewarping 
  An automatic dewarping mode. Works ONLY with such raw scans that have the top and 
  bottom curved page borders (on the black background). It automatically sets the red points 
  of the blue mesh along these borders (to create a distortion model) and then dewarps the scan 
  according to them. Works best on the low-curved scans. 
 
 
*Note: Other features of this version, such as Export, Dont_Equalize_Illumination_Pic_Zones, Original_Foreground_Mixed
has't been moved due to dirty realization. Their functionality is fully covered by 
[full control over settings on output](#full-control-over-settings-on-output) and 
[splitting output](#splitting-output) features.*

#### <a name="scantailor-advanced-features"></a> ScanTailor Advanced

* ##### ScanTailor Advanced fixes & improvements
    * Portability.  
      The settings and program files are stored in the folder with the application.  
      *Note: If installed into a system directory, where config and data files can't be written into the
      folder with the application executable, ScanTailor Advanced works as a standalone app and stores
      its settings and application data in the appropriate system specific paths.*
    
    * Page splitting settings influence on the output now by filling offcut.
      Fill offcut option has been added.
       
    * Page layout and all the other views now consider splitting settings.
      Corresponding improvements are done for thumbnails.
    
    * Changed ScanTailor behavior on page split stage.
         1. Reworked apply cut feature. Now on applying cut to the pages with different dimensions 
            than the page the cut applied to, ScanTailor tries to adapt cutters instead of fully
            rejecting the cut setting and switching to auto mode for those pages as it was before.
            The later was annoying as pages could be similar and had the difference in a few pixels.
         2. Added check to reject invalid cut settings in manual mode.
         3. UI: Added cutters interaction between each other. They can't more intersect each other,
            which created a wrong page layout configuration before.
    
    * Optimized memory usage on the output stage.
    
    * Reworking on [multi column thumbnails view](#multi-column-thumbnails-view-reworked) feature from ver. Enhanced. 
      Now thumbnails are shown evenly.
    
    * Added option to control highlighting the thumbnails of pages with high deviation with red asterisks. 
      The option refreshes the thumbnails instantly.
    
    * Deviation feature reworked.  
        1. A deviation provider implemented.  
           It supports caching and recalculates the values on demand. There isn't more any necessity to store deviation in page parameters and so in the project file, that approach caused some problems as the deviation is not actually a page parameter and depends on all the pages in the project.  
        2. Added sorting by decreasing deviation.
        
    * Auto and original alignment modes reworked:  
        1. The original and auto alignment modes didn't work correctly due to the error in code.
        2. Both the modes didn't work rightly after select content stage or reopening the project file, always requiring secondary batch processing of every page at margins stage to work correctly.
        3. Reworked calculation method for the original alignment. Now it is more precise.
        4. Original alignment mode now considers the page box from 4th stage.
        5. Fixed behaviour of horizontal alignment, when the original mode enabled, and auto margins has been enabled/disabled. Also on applying auto-margins / original alignment to the set of pages, that is now set correctly for each page.
        6. Added ability to separately control vertical and horizontal automatic alignment when auto or original alignment mode enabled.  

    * Changed the way of the adjustment of the despeckle strength.  
      Now that's set via the slider. It allows to adjust the despeckle strength more smoothly and exactly.
      Value 1.0 matches the old cautious mode, 2.0 - normal and 3.0 - aggressive.

    * Fixed other bugs of official, Enhanced and Featured versions and made lots of other improvements.

* ##### Light and Dark color schemes
  You can choose a desired color scheme in settings.

* ##### Multi-threading support for batch processing
  This significantly increases the speed of processing. The count of threads to use can be
  adjusted while processing.
  
  **Warning!** More threads requires more memory to use. Exclude situations of that to be overflowed.  

* ##### Full control over settings on output
  This feature enables to control filling margins, normalizing illumination before binarization,
  normalizing illumination in color areas and Savitzky-Golay and morphological smoothing options at the output stage
  in any mode (of course, those setting that can be applied in the current mode).
 
* ##### Filling outside areas
  Now outside pixels can be filled with the background color of the page.
  
  Added filling setting with the following options:
    1. Background: estimate the background and fill outside pixels with its color.
    2. White: always fill with white.
 
* ##### Tiff compression
  Tiff compression options allow to disable or change compression method in tiff files.
  
  There are two options in settings dialog: B&W and color compression. 
    1. The B&W one has None, LZW, Deflate and CCITT G4 (Default) options.
    2. The color one has None, LZW (Default), Deflate and JPEG options.

* ##### Adaptive binarization
  Sauvola and Wolf binarization algorithms have been added. They can be applied when
  normalizing illumination does not help.
 
* ##### Splitting output
  The feature allows to split the mixed output scans into the pairs of a foreground (letters) 
  and background (images) layer.
  
  You can choose between B&W or color (original) foreground.
  
  It can be useful:
    * for the further DjVu encoding,
    * to apply different filters to letters and images, which when being applied to the whole
    image gives worse results.
    * to apply a binarization to the letters from a third party app without affecting the images.
   
  *Note: That does not rename files to 0001, 0002... It can be made by a third party app, for example 
  [Bulk Rename Utility](http://www.bulkrenameutility.co.uk/Main_Intro.php)*

* ##### Original background
  This feature is a part of the [splitting output](#splitting-output) feature.
  
  It allows to preserve the original image background in the format ready for the further processing, when BW foreground is used.
  It can be used to encode into DjVu the pages with the complex background using the semi-auto "split layers" method which gives much higher quality results than DjVu auto segmenter.
  Also this feature can be used to extract high contrast elements of gradient images into the foreground layer by using second processing of the layer with pictures ("background").
  
  Properties of the original background:
    * Original background images are saved into "original_background" folder in "out" directory.
    * Pure black (`#000000`) and white (`#ffffff`) colors of original background image are reserved into `#010101` and `#fefefe`, respectively.
    * Picture zones are marked with black when the BW content is marked with white. This property allow to use "select by color" feature of an image editor to select needed areas for their further processing, for example, apply blur to white holes and their nearest areas to get an effective compression level of the background layer in DjVu.
    * Filling zones feature also removes trash and speckles from the original background when applied to the foreground layer.

* ##### Color segmenter and posterization
  Color segmentation and posterization (color quantization) features have been implemented.
  
  Color segmentation allows to split the image into color segments and colorize b&w mask.
  Posterization allows to reduce the number of colors of the image by grouping similar colors.
  The main use of posterization is to be applied to segmented image to get an indexed image, that can then be encoded into DjVu as the foreground layer. It allows to create high-quality DjVu files with color text and elements having maximal compression level.
  Posterization can also be used in color mode and can be applied to usual color gradient images for different purposes, for example, to increase their compression efficiency.

* ##### Rectangular picture shape
  "Quadro" picture shape mode from Featured was merged with Rectangular one from Enhanced. Also removed restriction of ver. Featured on deleting all the auto zones. Before it resulted in resetting all the auto zones back.
  Added sensitivity option. If sensitivity equals 25%, the results will be the same as they were in old "Quadro" mode, if 100% - as in old "Rectangular".

* ##### Page area
  This feature is a further development of the auto margins and page box features from version Enhanced.
  
  Selection content stage changes:
    1. A page box implementation reworked. Now it's interactive and can be adjusted by the same way as a content box is done.
    2. The page rectangle does not require refreshing page and won't be reset on the content area changes.
    3. The page rectangle is now drawn in the thumbnails.
    4. Implemented the new feature of applying the page box set manually to the other pages. For pages of a different size ST automatically corrects page box place. The same change has been made to applying the content box feature.
    5. Added a feature of dragging both content and page areas by using **`Shift+LMB`** combination.
    6. Added width and height parameters to regulate the page box size in manual mode.
    7. A page area is now allowed to be out of the page bounds in manual mode.
    
  Page layout stage changes:
    1. Auto margins option has been moved out of alignment settings and does no more force to use only the original layout.
    2. Auto margins feature now considers page box changes on the 4th stage.
    3. Corresponding bug fixes.
  
  Owing to the changes above, there appeared a new way to set margins for books and documents, pages of which have similar formatting. At content selection stage we just set page box and content box to a page, applying those content and page boxes to the pages with similar formatting, use auto margins for those pages at page layout stage instead of adjusting the margins relative to the content box, and then go to the output.

* ##### New zone interaction modes  
  **`Shift+LMB`** on a zone - drag the zone.  
  **`Ctrl+Shift+LMB`** on a zone - drag the zone copying.  
  **`Del`** when the cursor is over a zone - delete the zone.  
  Hold **`Ctrl`** when dragging a zone vertex - make the angle of the vertex right.  
  **`Ctrl+Alt+Click`** - copy the latest created zone to the cursor position.  
  **`Ctrl`** while creating a new zone - switch to rectangular mode.  
  Hold **`Shift+Alt+LMB`** while creating a new zone - use lasso (free drawing) mode.  

* ##### Saving zoom and focus on switching output tabs
  The save is precise and considers the images transformations.  
  Also added a feature of swithing the output tabs by using **`Ctrl+1..5`** keys combinations.  

* ##### Measurement units system
  The settings are available in the main window menu.
  Available units: pixels, millimetres, centimetres and inches.
  
  The system affects every aspect of the program, so, for example, it's now possible to adjust margins in pixels, but not only in millimetres or inches.

* ##### Status bar panel
  The panel shows the next information: mouse position relative to the image, physical size of the image, position of the selected page in current order and the page name and type (`[L]` or `[R]` - left or right page, if the page has been splitted).
  
  This feature is also affected by [measurement units system](#measurement-units-system).

* ##### Default parameters
  Default parameters system supporting custom profiles has been implemented.
  
  The system allows to manage the default filter settings for every stage.
  Those filter parameters will be set as defaults for any new project created.
  
  For example, it allows to set your own default margins standard, but not default 5, 10, 5, 10 mm, and so for the other parameters.
  
  Peculiarities:
    1. There are two default profiles: "Default" and "Source". The "Default" profile represents default ST filter settings, the "Source" one represents the settings giving the source as output without any changes.
    2. A user can create its own profiles. User profiles are stored in `config/profiles` folder or in an system specific one for application data.
    3. The system consider the units settings from the [measurement units system](#measurement-units-system). Units are stored in the profile and ST automatically converts the values if needed.

* ##### Collapsible filter options.
  Now group boxes containing filter options can be collapsed/expanded.  
  The collapse status is preserved between restarts of the application.  
 
* ##### Auto adjusting content area.
  Use **`double-click`** on content to automatically adjust the content area.  
  If the content is outside the area, the later will automatically be expanded and adjusted to the content at the position where double-click has been,
  otherwise the area edge, nearest to that position, will be adjusted.
  
  It's much faster now to correct the content area if, for example, the page number has been missed by the auto algorithm.
  It is no more required to manually and laboriously move the corners and edges of the content box.
  
* ##### Black on white detection
  This feature allows to process images with light content on dark background correctly by correcting auto algorithms.  
  
  Peculiarities:
    1. Auto detection of pages with light content on dark background can be enabled or disabled in the settings.
       Auto detection at the output stage is controlled separately.
    2. There is per page control over the mode in the output filter options.
    
* ##### Guides
  This feature gives you a more flexible and precise way of positioning content in the page layout.
  These are horizontal or vertical lines you can display on a page at the margins stage \(when aligning enabled\).  
  *Note: Guides are adaptive to the page soft margins, i.e. when the latter changed the guides on the page
  are automatically adjusted to match the new content position without requiring any manual re-adjusting.*
  
  Capabilities:  
    * **`Right-click`** to create/remove guides from the **context menu** called.  
    * **`Right-click`** on a guide to delete that guide from the **context menu** called.  
    * **`Shift+LMB`** - drag the guide under the cursor.  
    * **`Shift/Ctrl+LMB`** on the content rectangle - drag the page content.
      Hold **`Shift`** pressed to restrict moving along the horizontal axis only or **`Ctrl`** for the vertical one.
      Hold **`Shift+Ctrl`** for usual dragging.  
    * **`Double-click`** on content - automatically attach that content to the nearest guide.
      Hold **`Shift`** pressed to select vertical guides only or **`Ctrl`** for horizontal ones.
      Hold **`Shift+Ctrl`** to attach that to both the nearest vertical and horizontal guides.  
    * Use the **context menu** to enable/disable showing the hard margins rectangle.  

Building
----------

Go to [this repository](https://github.com/4lex4/scantailor-libs-build) and follow the instructions given there.
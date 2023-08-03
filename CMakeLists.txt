cmake_minimum_required(VERSION 3.16)
project(WebPdf)

# Setup the Targoman CMake build system
include(./3rdParty/TargomanCMake/targoman.cmake)
tg_setup_buildsystem_paths()

# Global compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3rdParty folder (Even though the libraries are not actually 3rd party :) )
add_subdirectory(3rdParty)

# OpenCV
if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
find_package(OpenCV REQUIRED)
endif()

# Main library
tg_add_library(pdfla
    STATIC
    libsrc/clsPdfDocument.cpp
    libsrc/Private/clsBezierCurve.cpp
    libsrc/Private/clsColumn.cpp
    libsrc/Private/clsLayoutAnalyzer.cpp
    libsrc/Private/clsLayoutStorage.cpp
    libsrc/Private/clsPdfBlock.cpp
    libsrc/Private/clsPdfChar.cpp
    libsrc/Private/clsPdfFont.cpp
    libsrc/Private/clsPdfItem.cpp
    libsrc/Private/clsPdfiumWrapper.cpp
    libsrc/Private/clsPdfLineSegment.cpp
    libsrc/Private/common.cpp
    libsrc/Private/LayoutAnalysis/BlockCreation.cpp
    libsrc/Private/LayoutAnalysis/BulletAndNumbering.cpp
    libsrc/Private/LayoutAnalysis/HeaderFooterAndSideBar.cpp
    libsrc/Private/LayoutAnalysis/LACommon.cpp
    libsrc/Private/LayoutAnalysis/PdfLineSegmentManager.cpp
    libsrc/Private/LayoutAnalysis/Stamping.cpp
    libsrc/Private/LayoutAnalysis/SubBlockMarking.cpp
    libsrc/Private/ParagraphAndSentenceManager.cpp
)

tg_add_library_headers(pdfla
    PUBLIC_HEADER
    libsrc/clsPdfDocument.h
    libsrc/common.h
    libsrc/stuMarkableSegment.h
)

tg_add_library_headers(pdfla
    PRIVATE_HEADER
    libsrc/Private/clsBezierCurve.h
    libsrc/Private/clsColumn.h
    libsrc/Private/clsLayoutAnalyzer.h
    libsrc/Private/clsLayoutStorage.h
    libsrc/Private/clsPdfBlock.h
    libsrc/Private/clsPdfChar.cpp
    libsrc/Private/clsPdfChar.h
    libsrc/Private/clsPdfFont.h
    libsrc/Private/clsPdfItem.h
    libsrc/Private/clsPdfiumWrapper.h
    libsrc/Private/clsPdfLineSegment.h
    libsrc/Private/common.h
    libsrc/Private/LayoutAnalysis/BlockCreation.h
    libsrc/Private/LayoutAnalysis/BulletAndNumbering.h
    libsrc/Private/LayoutAnalysis/HeaderFooterAndSideBar.h
    libsrc/Private/LayoutAnalysis/LACommon.h
    libsrc/Private/LayoutAnalysis/PdfLineSegmentManager.h
    libsrc/Private/LayoutAnalysis/Stamping.h
    libsrc/Private/LayoutAnalysis/SubBlockMarking.h
    libsrc/Private/ParagraphAndSentenceManager.cpp
    libsrc/Private/ParagraphAndSentenceManager.h
)

message("CMAKE_SYSTEM_NAME:${CMAKE_SYSTEM_NAME}")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
add_executable(webpdf
    wasm/interface.cpp
)
target_link_libraries(webpdf
    pdfla
    fpdfapi
    fdrm
    fpdfdoc
    fpdftext
    fxcodec
    fxcrt
    fxge    
)
target_compile_options(webpdf
    PUBLIC
    -flto
    -fno-exceptions
)
target_link_options(webpdf
    PUBLIC
    -Oz
    -sASSERTIONS=1
    -sELIMINATE_DUPLICATE_FUNCTIONS=1
    # -sFILESYSTEM=0
    -sMODULARIZE=1
    -sEXPORTED_FUNCTIONS=["_malloc","_free"]
    -sTOTAL_MEMORY=256MB
    -flto
    --bind
    --post-js ${CMAKE_CURRENT_SOURCE_DIR}/wasm/post.js
    --closure 0
)
add_custom_command(
    OUTPUT
    wcomp/dist/webpdfui.min.css wcomp/dist/webpdfui.min.js
    COMMAND
    wcomp/build.sh
    DEPENDS
    webpdf
    wcomp/src/clsTargomanPdfViewer.mjs 
    wcomp/src/modules/clsEventTarget.mjs
    wcomp/src/modules/clsPdfDocumentViewer.mjs
    wcomp/src/modules/clsTargomanPdfDocumentInterface.mjs
    wcomp/src/modules/clsTargomanPdfViewerPrivate.mjs
    wcomp/src/modules/common.mjs
    wcomp/src/modules/l10n.mjs
    wcomp/src/modules/UI.mjs    
)
endif()

# Finalize the settings
tg_process_all_targets()

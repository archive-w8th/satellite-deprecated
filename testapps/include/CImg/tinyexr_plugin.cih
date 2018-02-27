/*
 #
 #  File        : add_fileformat.h
 #                ( C++ header file - CImg plug-in )
 #
 #  Description : CImg plug-in that adds loading/saving support for a personalized
 #                file format (determined by its extension, here ".foo").
 #                This file is a part of the CImg Library project.
 #                ( http://cimg.eu )
 #
 #  Copyright   : David Tschumperle
 #                ( http://tschumperle.users.greyc.fr/ )
 #
 #  License     : CeCILL v2.0
 #                ( http://www.cecill.info/licences/Licence_CeCILL_V2-en.html )
 #
 #  This software is governed by the CeCILL  license under French law and
 #  abiding by the rules of distribution of free software.  You can  use,
 #  modify and/ or redistribute the software under the terms of the CeCILL
 #  license as circulated by CEA, CNRS and INRIA at the following URL
 #  "http://www.cecill.info".
 #
 #  As a counterpart to the access to the source code and  rights to copy,
 #  modify and redistribute granted by the license, users are provided only
 #  with a limited warranty  and the software's author,  the holder of the
 #  economic rights,  and the successive licensors  have only  limited
 #  liability.
 #
 #  In this respect, the user's attention is drawn to the risks associated
 #  with loading,  using,  modifying and/or developing or reproducing the
 #  software by the user in light of its specific status of free software,
 #  that may mean  that it is complicated to manipulate,  and  that  also
 #  therefore means  that it is reserved for developers  and  experienced
 #  professionals having in-depth computer knowledge. Users are therefore
 #  encouraged to load and test the software's suitability as regards their
 #  requirements in conditions enabling the security of their systems and/or
 #  data to be ensured and,  more generally, to use and operate it in the
 #  same conditions as regards security.
 #
 #  The fact that you are presently reading this means that you have had
 #  knowledge of the CeCILL license and that you accept its terms.
 #
*/

#ifndef cimg_plugin_tinyexr_adv
#define cimg_plugin_tinyexr_adv
//#include "CImg.h"
//#include "tinyexr.h"

    //using namespace cil;

    //---------------------------------------
    inline CImg<T>& get_load_exr_adv(const char *const filename) {
        EXRVersion exr_version;
        int ret = ParseEXRVersionFromFile(&exr_version, filename);
        if (ret != 0) { fprintf(stderr, "Invalid EXR file: %s\n", filename); return *this; }
        if (exr_version.multipart) { return *this; }

        EXRHeader exr_header;
        InitEXRHeader(&exr_header);

        const char* err;
        ret = ParseEXRHeaderFromFile(&exr_header, &exr_version, filename, &err);
        if (ret != 0) { fprintf(stderr, "Parse EXR err: %s\n", err); return *this; }

        // read as float 
        for (int c = 0; c < exr_header.num_channels; c++) { exr_header.requested_pixel_types[c] = TINYEXR_PIXELTYPE_FLOAT; }

        // read image
        EXRImage exr_image;
        InitEXRImage(&exr_image);
        ret = LoadEXRImageFromFile(&exr_image, &exr_header, filename, &err);
        if (ret != 0) { fprintf(stderr, "Load EXR err: %s\n", err); return *this; }

        // search channels
        int rc = 0, gc = 1, bc = 2, ac = 3; // defaultly should be ABGR, but used RGBA
        for (int c = 0; c < exr_header.num_channels; c++) {
            if (exr_header.channels[c].name[0] == 'R') rc = c;
            if (exr_header.channels[c].name[0] == 'G') gc = c;
            if (exr_header.channels[c].name[0] == 'B') bc = c;
            if (exr_header.channels[c].name[0] == 'A') ac = c;
        }

        // copy channels linearly
        CImg<floatT> _img(exr_image.width, exr_image.height, 1, exr_header.num_channels, true);
        if (rc >= 0 && rc < exr_header.num_channels) memcpy(_img.get_shared_channel(0).data(), exr_image.images[rc], exr_image.width * exr_image.height * (exr_header.pixel_types[rc] == TINYEXR_PIXELTYPE_FLOAT ? 4 : 2));
        if (gc >= 0 && gc < exr_header.num_channels) memcpy(_img.get_shared_channel(1).data(), exr_image.images[gc], exr_image.width * exr_image.height * (exr_header.pixel_types[gc] == TINYEXR_PIXELTYPE_FLOAT ? 4 : 2));
        if (bc >= 0 && bc < exr_header.num_channels) memcpy(_img.get_shared_channel(2).data(), exr_image.images[bc], exr_image.width * exr_image.height * (exr_header.pixel_types[bc] == TINYEXR_PIXELTYPE_FLOAT ? 4 : 2));
        if (ac >= 0 && ac < exr_header.num_channels) memcpy(_img.get_shared_channel(3).data(), exr_image.images[ac], exr_image.width * exr_image.height * (exr_header.pixel_types[ac] == TINYEXR_PIXELTYPE_FLOAT ? 4 : 2));
        FreeEXRImage(&exr_image);
        return _img.move_to(*this);
    }

    inline CImg& load_exr_adv(const char *const filename)
    {
        return get_load_exr_adv(filename).swap(*this);
    }

    // This function saves the instance image into a ".exr" file.
    //-----------------------------------------------------------
    inline const CImg& save_exr_adv(const char *const filename) const
    {
        EXRHeader header;
        InitEXRHeader(&header);

        EXRImage image;
        InitEXRImage(&image);

        const floatT* image_ptr[4];
        image_ptr[0] = (const floatT*)this->get_shared_channel(3).data();
        image_ptr[1] = (const floatT*)this->get_shared_channel(2).data();
        image_ptr[2] = (const floatT*)this->get_shared_channel(1).data();
        image_ptr[3] = (const floatT*)this->get_shared_channel(0).data();

        image.num_channels = 4;
        image.width = this->width();
        image.height = this->height();
        image.images = (unsigned char**)image_ptr;

        header.num_channels = 4;
        header.channels = (EXRChannelInfo *)malloc(sizeof(EXRChannelInfo) * header.num_channels);
        strncpy(header.channels[0].name, "A", 255); header.channels[0].name[strlen("A")] = '\0';
        strncpy(header.channels[1].name, "B", 255); header.channels[1].name[strlen("B")] = '\0';
        strncpy(header.channels[2].name, "G", 255); header.channels[2].name[strlen("G")] = '\0';
        strncpy(header.channels[3].name, "R", 255); header.channels[3].name[strlen("R")] = '\0';

        header.pixel_types = (int*)malloc(sizeof(int) * header.num_channels);
        header.requested_pixel_types = (int *)malloc(sizeof(int) * header.num_channels);
        for (int i = 0; i < header.num_channels; i++) {
            header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // only supported float
            header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // save as float
        }

        const char* err;
        int ret = SaveEXRImageToFile(&image, &header, filename, &err);
        if (ret != TINYEXR_SUCCESS) {
            fprintf(stderr, "Save EXR err: %s\n", err);
            return *this;
        }
        printf("Saved exr file. [ %s ] \n", filename);
        free(header.channels);
        free(header.pixel_types);
        free(header.requested_pixel_types);
        return *this;
    }

    // The code below allows to add the support for the specified extension.
    //---------------------------------------------------------------------
#ifndef cimg_load_plugin
#define cimg_load_plugin(filename) \
  if (!cimg::strncasecmp(cimg::split_filename(filename),"exr",3)) return load_exr_adv(filename);
#endif
#ifndef cimg_save_plugin
#define cimg_save_plugin(filename) \
  if (!cimg::strncasecmp(cimg::split_filename(filename),"exr",3)) return save_exr_adv(filename);
#endif

// End of the plugin.
//-------------------
#endif /* cimg_plugin_tinyexr_adv */

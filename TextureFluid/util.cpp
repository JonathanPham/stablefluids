#include <util.h>
#include <png.h>
#include <string>

bool read_png(std::string png_filename,
              void**      pixel_data,
              size_t*     width,
              size_t*     height,
              bool        internal_format_rgba)
{
    if(!pixel_data || !width || !height) {
        return false;
    }
    if(!internal_format_rgba) {
        return read_png(png_filename,
                        pixel_data,
                        width,
                        height);
    }
    unsigned char* src_pixel_data = NULL;
    size_t src_width  = 0;
    size_t src_height = 0;
    if(!read_png(png_filename,
                 reinterpret_cast<void**>(&src_pixel_data),
                 &src_width,
                 &src_height) || !src_pixel_data)
    {
        return false;
    }
    unsigned char* dest_pixel_data = NULL;
    size_t size_buf = src_width * src_height * sizeof(unsigned char) * 4;
    dest_pixel_data = new unsigned char[size_buf];
    if(!dest_pixel_data) {
        return false;
    }
    for(int y = 0; y < static_cast<int>(src_height); y++) {
        for(int x = 0; x < static_cast<int>(src_width); x++) {
            int src_pixel_offset_scanline_start  = (y * src_width + x) * 3;
            int dest_pixel_offset_scanline_start = (y * src_width + x) * 4;
            dest_pixel_data[dest_pixel_offset_scanline_start + 0] = src_pixel_data[src_pixel_offset_scanline_start + 0];
            dest_pixel_data[dest_pixel_offset_scanline_start + 1] = src_pixel_data[src_pixel_offset_scanline_start + 1];
            dest_pixel_data[dest_pixel_offset_scanline_start + 2] = src_pixel_data[src_pixel_offset_scanline_start + 2];
            dest_pixel_data[dest_pixel_offset_scanline_start + 3] = 1;
        }
    }
    delete []src_pixel_data;
    *pixel_data = dest_pixel_data;
    *width      = src_width;
    *height     = src_height;
    return true;
}

bool read_png(std::string png_filename,
              void**      pixel_data,
              size_t*     width,
              size_t*     height)
{
    if(!pixel_data || !width || !height) {
        return false;
    }

    // header for testing if it is a png
    png_byte header[8];

    // open file as binary
    FILE* fp = fopen(png_filename.c_str(), "rb");
    if(!fp) {
        return false;
    }

    // read the header
    fread(header, 1, 8, fp);

    // test if png
    int is_png = !png_sig_cmp(header, 0, 8);
    if(!is_png) {
        fclose(fp);
        return false;
    }

    // create png struct
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png_ptr) {
        fclose(fp);
        return false;
    }

    // create png info struct
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
        fclose(fp);
        return false;
    }

    // create png info struct
    png_infop end_info = png_create_info_struct(png_ptr);
    if(!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
        fclose(fp);
        return false;
    }

    // png error stuff, not sure libpng man suggests this
    if(setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return false;
    }

    // init png reading
    png_init_io(png_ptr, fp);

    // let libpng know you already read the first 8 bytes
    png_set_sig_bytes(png_ptr, 8);

    // read all the info up to the image data
    png_read_info(png_ptr, info_ptr);

    // variables to pass to get info
    int bit_depth, color_type;
    png_uint_32 twidth, theight;

    // get info about png
    png_get_IHDR(png_ptr, info_ptr, &twidth, &theight, &bit_depth, &color_type, NULL, NULL, NULL);

    // update the png info struct.
    png_read_update_info(png_ptr, info_ptr);

    // row size in bytes.
    int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

    // allocate the image_data as a big block, to be given to opengl
    png_byte* image_data = new png_byte[rowbytes * theight];
    if(!image_data) {
        // clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        fclose(fp);
        return false;
    }

    // row_pointers is for pointing to image_data for reading the png with libpng
    png_bytep* row_pointers = new png_bytep[theight];
    if(!row_pointers) {
        //clean up memory and close stuff
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        delete[] image_data;
        fclose(fp);
        return false;
    }
    // set the individual row_pointers to point at the correct offsets of image_data
    for(int i = 0; i < static_cast<int>(theight); i++) {
        row_pointers[theight - 1 - i] = image_data + i * rowbytes;
    }

    // read the png into image_data through row_pointers
    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;
    fclose(fp);

    // clean up memory and close stuff
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

    // update width and height based on png info
    *pixel_data = image_data;
    *width      = twidth;
    *height     = theight;

    return true;
}

#include <stdio.h>
#define MEM_IMP
#define NOB_IMPLEMENTATION
#include "../headers/nob.h"
#define OUTPUT_FILE "./build/bundle.h"
typedef struct Resource {
    const char *file_path;
    size_t offset;
    size_t size;
}Resource;
Resource resources[]={
    {.file_path="./fonts/IosevkaNerdFontMono-Regular.ttf"},
};
int main(void) {
    Nob_String_Builder bundle={0},content={0};
    content.count=0;
    for (size_t i=0;i<NOB_ARRAY_LEN(resources);++i) {
        if(!nob_read_entire_file(resources[i].file_path,&content))
            return -1;
        resources[i].offset=bundle.count;
        resources[i].size=content.count;
        nob_da_append_many(&bundle,content.items,content.count);
    }
    FILE *f=fopen(OUTPUT_FILE,"wb");
    fprintf(f,"typedef struct Resource {\n const char *file_path;\n size_t offset;\n size_t size;\n}Resource;\n");
    fprintf(f,"size_t resources_count = %zu;\n",NOB_ARRAY_LEN(resources));
    fprintf(f,"Resource resources[] = {\n");
    for (size_t i=0;i<NOB_ARRAY_LEN(resources);++i) {
    fprintf(f," {.file_path=\"%s\", .offset= %zu, .size= %zu},\n",
            resources[i].file_path,resources[i].offset,resources[i].size);
    }
    fprintf(f,"};\n");
    size_t row_size=20;
    fprintf(f,"unsigned char bundle[] = {\n");
    for (size_t row=0;row<bundle.count/row_size;++row) {
        for (size_t col=0;col<row_size;++col) {
            fprintf(f,"    ");
            size_t i=row*row_size+col;
            fprintf(f,"0x%02X, ",(unsigned char)bundle.items[i]);
        }
        fprintf(f,"\n");
    }
    size_t rema=bundle.count%row_size;
    if (rema>0) {
         for (size_t col=0;col<row_size;++col) {
            fprintf(f,"    ");
            size_t i=(bundle.count/row_size)*row_size+col;
            fprintf(f,"0x%02X, ",(unsigned char)bundle.items[i]);
        }
    }
    fprintf(f,"};\n");

    fclose(f);
}

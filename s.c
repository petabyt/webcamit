#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void stream_image(const char *filename) {
    FILE *img_file = fopen(filename, "rb");
    if (!img_file) {
        perror("Unable to open image file");
        exit(1);
    }

    unsigned char buffer[1024];
    size_t bytes_read;

    while (1) {
        // Read the image file and output it to stdout
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), img_file)) > 0) {
            if (fwrite(buffer, 1, bytes_read, stdout) != bytes_read) {
                perror("Error writing to stdout");
                fclose(img_file);
                exit(1);
            }
        }

        // Rewind the file to the start for the next frame
        fseek(img_file, 0, SEEK_SET);
        fflush(stdout);  // Ensure output is flushed to stdout

        usleep(100000);  // Sleep for 100ms (adjust frame rate as needed)
    }

    fclose(img_file);
}

int main() {
    const char *filename = "lv.jpg";  // Image to stream
    stream_image(filename);
    return 0;
}

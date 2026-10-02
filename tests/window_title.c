#include <MiniFB.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Basic test data for window titles
static const char * const basic_data[] = {
    "Test",
    "Hello, World!",
    "!!! MiniFB MiniFB MiniFB MiniFB MiniFB MiniFB !!!",
    "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolor"
    "e magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd"
    " gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipsc"
    "ing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At v"
    "ero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem "
    "ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invid"
    "unt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea "
    "rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Duis autem vel eum iriur"
    "e dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at ve"
    "ro eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feuga"
    "it nulla facilisi. Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidu"
    "nt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamco"
    "rper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vu"
    "lputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et ius"
    "to odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. Nam lib"
    "er tempor cum soluta nobis eleifend option congue nihil imperdiet doming id quod mazim placerat facer possim assum"
    ". Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dol"
    "ore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit"
};

// Extended test data for window titles with Unicode characters
// DISCLAIMER: AI came up with the test data (that's the only thing AI touched in this file)
static const char * const extended_data[] = {
    "Ελλάδα",                  // Pure 2-byte UTF-8 (Greek: "Greece")
    "Café résumé",             // Mixed 1-byte and 2-byte UTF-8
    "こんにちは",               // Pure 3-byte UTF-8 (Japanese hiragana: "hello")
    "Hello 世界",               // Mixed 1-byte and 3-byte UTF-8 ("Hello world")
    "𝐇𝐞𝐥𝐥𝐨",                   // Pure 4-byte UTF-8 (Mathematical Bold: "Hello")
    "Hello 𝐖𝐨𝐫𝐥𝐝",             // Mixed 1-byte and 4-byte UTF-8
    "Hi Café 你好 😊 𝐎𝐊",       // Mixed 1/2/3/4-byte UTF-8 sequences
};

static bool test_window_title(struct mfb_window *window, const char *title) {
    if (window == NULL) {
        fprintf(stdout, "test_window_title: window is NULL -- this really shouldn't be happening\n");
        fflush(stdout);
        return false;
    }

    if (title == NULL) {
        fprintf(stdout, "test_window_title: title is NULL -- this really shouldn't be happening\n");
        fflush(stdout);
        return false;
    }

    // Set the title

    // Copy the title to a local buffer
    size_t length = strlen(title);
    char *buffer = (char *)malloc(length + 1);
    if (buffer == NULL) {
        fprintf(stdout, "test_window_title: failed to allocate memory for title buffer\n");
        fflush(stdout);
        return false;
    }
    memcpy(buffer, title, length + 1); // Copy the title including the null terminator

    // Set the window title using the copied buffer
    mfb_set_title(window, buffer);

    // Might be required for some platforms to update the title, I'm not sure
    mfb_wait_sync(window);

    fprintf(stdout, "mfb_set_title: \"%s\"\n", buffer);
    fflush(stdout);

    // Free the copied buffer
    free(buffer);
    buffer = NULL;

    // Read back the set title

    // Step 1: Get the required buffer size for the window title
    mfb_string_result result = mfb_get_title(window, NULL, 0);
    if (result < MFB_STRING_EMPTY) {
        switch (result) {
            case MFB_STRING_INVALID_WINDOW:
                fprintf(stdout, "mfb_get_title: MFB_STRING_INVALID_WINDOW\n");
                break;
            case MFB_STRING_INVALID_ARGUMENT:
                fprintf(stdout, "mfb_get_title: MFB_STRING_INVALID_ARGUMENT\n");
                break;
            case MFB_STRING_BUFFER_TOO_SMALL:
                fprintf(stdout, "mfb_get_title: MFB_STRING_BUFFER_TOO_SMALL\n");
                break;
            case MFB_STRING_INTERNAL_ERROR:
                fprintf(stdout, "mfb_get_title: MFB_STRING_INTERNAL_ERROR\n");
                break;
            default:
                fprintf(stdout, "mfb_get_title: unknown error\n");
                break;
        }
        fflush(stdout);
        return false;
    }
    fprintf(stdout, "mfb_get_title: %d bytes\n", result);
    fflush(stdout);

    // Step 2: Allocate a buffer of the required size
    buffer = (char *)malloc(result + 1); // mfb_get_title returns the required buffer size, excluding the null terminator
    if (buffer == NULL) {
        fprintf(stdout, "test_window_title: failed to allocate memory for title buffer\n");
        fflush(stdout);
        return false;
    }

    // Step 3: Get the window title
    result = mfb_get_title(window, buffer, result + 1);
    if (result < MFB_STRING_EMPTY) {
        switch (result) {
            case MFB_STRING_INVALID_WINDOW:
                fprintf(stdout, "mfb_get_title: MFB_STRING_INVALID_WINDOW\n");
                break;
            case MFB_STRING_INVALID_ARGUMENT:
                fprintf(stdout, "mfb_get_title: MFB_STRING_INVALID_ARGUMENT\n");
                break;
            case MFB_STRING_BUFFER_TOO_SMALL:
                fprintf(stdout, "mfb_get_title: MFB_STRING_BUFFER_TOO_SMALL\n");
                break;
            case MFB_STRING_INTERNAL_ERROR:
                fprintf(stdout, "mfb_get_title: MFB_STRING_INTERNAL_ERROR\n");
                break;
            default:
                fprintf(stdout, "mfb_get_title: unknown error\n");
                break;
        }
        fflush(stdout);
        free(buffer);
        return false;
    }
    fprintf(stdout, "mfb_get_title: \"%s\"\n", buffer);
    fflush(stdout);

    // Compare the retrieved title with the expected title
    bool matches = (strcmp(buffer, title) == 0);
    if (!matches) {
        fprintf(stdout, "mfb_set_title/mfb_get_title: title mismatch\n");
        fflush(stdout);
    }

    // Free the allocated buffer
    free(buffer);
    buffer = NULL;

    return matches;
}

static bool run_tests(struct mfb_window *window, int* total, int* pass, bool interactive) {
    if (total != NULL) { *total = 0; }
    if (pass != NULL) { *pass = 0; }

    if ((sizeof(basic_data) / sizeof(basic_data[0])) > 0) {
        fprintf(stdout, "Basic title tests\n");
        fprintf(stdout, "------------------------------------------------------------\n");
        fprintf(stdout, "\n");
        fflush(stdout);
        for (unsigned int i = 0; i < sizeof(basic_data) / sizeof(basic_data[0]); i++) {
            if (interactive) {
                int response = 0;
                while (response != '\r' && response != '\n' && response != 'y' && response != 'Y')
                {                    
                    fprintf(stdout, "Continue? [Y/n]: ");
                    fflush(stdout);

                    response = fgetc(stdin);
                    
                    if (response == EOF) {
                        fprintf(stdout, "Failed to read input\n");
                        return false;
                    }
                    
                    // Discard the rest of the line
                    if (response != '\n') {
                        int ch;
                        while ((ch = fgetc(stdin)) != '\n' && ch != EOF);
                    }

                    if (response == 'n' || response == 'N') {
                        return true;
                    }
                }
            }

            if (total != NULL) (*total)++;
            if (test_window_title(window, basic_data[i])) {
                if (pass != NULL) (*pass)++;
                fprintf(stdout, "Test %d: PASS\n", i + 1);
            } else {
                fprintf(stdout, "Test %d: FAIL\n", i + 1);
            }        
        }
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    if ((sizeof(extended_data) / sizeof(extended_data[0])) > 0) {
        fprintf(stdout, "Extended title tests\n");
        fprintf(stdout, "------------------------------------------------------------\n");
        fprintf(stdout, "\n");
        fflush(stdout);
        for (unsigned int i = 0; i < sizeof(extended_data) / sizeof(extended_data[0]); i++) {
            if (interactive) {
                int response = 0;
                while (response != '\r' && response != '\n' && response != 'y' && response != 'Y')
                {                    
                    fprintf(stdout, "Continue? [Y/n]: ");
                    fflush(stdout);

                    response = fgetc(stdin);
                    
                    if (response == EOF) {
                        fprintf(stdout, "Failed to read input\n");
                        return false;
                    }
                    
                    // Discard the rest of the line
                    if (response != '\n') {
                        int ch;
                        while ((ch = fgetc(stdin)) != '\n' && ch != EOF);
                    }

                    if (response == 'n' || response == 'N') {
                        return true;
                    }
                }
            }

            if (total != NULL) { (*total)++; }
            if (test_window_title(window, extended_data[i])) {
                if (pass != NULL) { (*pass)++; }
                fprintf(stdout, "Test %d: PASS\n", i + 1);
            } else {
                fprintf(stdout, "Test %d: FAIL\n", i + 1);
            }        
        }
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    return true;
}

int
main(int argc, char *argv[]) {
    bool interactive = false;
    if (argc > 1) {
        if ((strcmp(argv[1], "-i") == 0) || (strcmp(argv[1], "--interactive") == 0)) {
            interactive = true;
        }
    }

    fprintf(stdout, "MiniFB Window title setting and getting test\n");
    fprintf(stdout, "============================================================\n");
    if (interactive) {
        fprintf(stdout, "Running in interactive mode\n");
    }
    fprintf(stdout, "\n");
    fflush(stdout);

    struct mfb_window *window = mfb_open("Test Window", 800, 600);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        return 1;
    }

    mfb_wait_sync(window);

    int total = 0;
    int pass = 0;
    
    if (!run_tests(window, &total, &pass, interactive)) {
        mfb_close(window);
        return 1;
    }
    
    int fail = total - pass;
    fprintf(stdout, "============================================================\n");
    fprintf(stdout, "Total tests: %d, Passed: %d, Failed: %d\n", total, pass, fail);
    fflush(stdout);

    mfb_close(window);

    return fail;
}
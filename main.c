#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fftw3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>

#define PORT 7373
//#define FFT_SIZE (MAX_BUFFER_SIZE / sizeof(float) / 2)  // Number of complex samples for float
#define FFT_SIZE 16384
#define FFT_BYTES FFT_SIZE * sizeof(int32_t) * 2
#define MAX_BUFFER_SIZE FFT_BYTES + 16384
#define FONTSIZE 15

SDL_Surface *screen; 
SDL_Window *window;
SDL_Surface *window_surface, *textSurface;
TTF_Font *font;

void create_spectrum_window();
void plot_spectrum(double *spectrum, int size);

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    unsigned int buff_len = 0;
    int recv_len = 0;
    int32_t in_buffer[FFT_SIZE*2];  // Buffer to hold incoming data
    fftw_complex *in, *out;
    fftw_plan p;

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return EXIT_FAILURE;
    }

    create_spectrum_window();

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FFT_SIZE);

    // FFT setup
    p = fftw_plan_dft_1d(FFT_SIZE, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    printf("Listening for UDP packets on port %d...\n", PORT);

    while (1) {
        uint8_t running = 1;

        if (buff_len < FFT_BYTES) {
            recv_len = recvfrom(sockfd, (uint8_t*)in_buffer + buff_len, sizeof(in_buffer)-buff_len, 0, (struct sockaddr *)&client_addr, &addr_len);
            if (recv_len < 0) {
                perror("Receive failed");
                break;
            }

            printf("Received %d bytes\n", recv_len);
            buff_len += recv_len;
        }
        else {
            buff_len=0;
//        // For demonstration, we just print out the first few samples
//        int sample_count = recv_len / sizeof(float);
//        printf("First %d samples received: ", sample_count);
//        for (int i = 0; i < (sample_count < 10 ? sample_count : 10); ++i) {
//            printf("%f ", float_buffer[i]);
//        }
//        printf("\n");
//

        
            // Convert interleaved float_buffer to complex double buffer
            for (int i = 0; i < FFT_SIZE; i++) {
                in[i][0] = in_buffer[2*i]/(double)2147483647.0;  // Convert each int32 to double in range -1 to 1
                in[i][1] = in_buffer[2*i+1]/(double)2147483647.0;  // Convert each int32 to double  in range -1 to 1
                
                // Apply Hanning window
                in[i][0] *= 0.5 * (1 - cos(2 * M_PI * i / (FFT_SIZE - 1)));
                in[i][1] *= 0.5 * (1 - cos(2 * M_PI * i / (FFT_SIZE - 1)));
            }
        
            // Execute FFT
            fftw_execute(p);

            // Calculate magnitude in dB
            double magnitude[FFT_SIZE];
            for (size_t i = 0; i < FFT_SIZE; i++) {
                double mag = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1])/FFT_SIZE;
                magnitude[i] = (mag > 0) ? 20 * log10(mag) : -150.0;  // Avoid log10(0)
            }

            // Plot the spectrum
            plot_spectrum(magnitude, FFT_SIZE);

            
        }
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                   running = 0;  // Exit on quit event
            }
            else if (event.type == SDL_KEYDOWN) {
                // Print which key was pressed
                //printf("Key pressed: %s\n", SDL_GetKeyName(event.key.keysym.sym));
                if (event.key.keysym.sym == SDLK_SPACE) {
                    if (IMG_SavePNG(screen, "spectrum.png") == 0)
                        printf("Saving spectrum as spectrum.png\n");
                    else
                        printf("Cannot save spectrum.png\n");
               }
            }
        }
        if (running == 0)
            break; 
    }

    // Cleanup
    fftw_destroy_plan(p);
    fftw_free(out);

    // Cleanup
    SDL_FreeSurface(screen);
    SDL_DestroyWindow(window);
    SDL_FreeSurface(textSurface); 
    TTF_CloseFont(font);          
    TTF_Quit();  
    SDL_Quit();
    
    close(sockfd);
    return EXIT_SUCCESS;
}

//void plot_spectrum(double *spectrum, int size) {
//    SDL_Init(SDL_INIT_VIDEO);
//    SDL_Window *window = SDL_CreateWindow("FFT Spectrum (dB)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);
//    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
//
//    // Setup for visualization
//    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
//    SDL_RenderClear(renderer);
//    
//    // Find the maximum magnitude in the spectrum for scaling
//    //double max_magnitude = -150.0; // Initialize to minimum expected dB
//    //for (int i = 0; i < size; i++) {
//    //    if (spectrum[i] > max_magnitude) {
//    //        max_magnitude = spectrum[i];
//    //    }
//    //}
//
//    // Scaling factor to map dB to pixel height
//    //double scale_range = max_magnitude + 150;  // To map to a visible range
//    double scaling_factor = 600.0 / 150; // Scale height to window's height  
//
//    for (int i = 0; i < size; i++) {
//        // Normalize dB values to the height range
//        double normalized_height = (spectrum[i] + 150.0) * scaling_factor; // Shift by 150 dB for visibility
//        int height = (int)normalized_height;
//
//        if (height < 0) height = 0;  // Ensure height doesn't drop below 0
//        if (height > 600) height = 600; // Limit height to window's height
//
//        // Debug print lines
//        printf("Index: %d, dB Value: %.2f, Height: %d\n", i, spectrum[i], height);
//
//        SDL_SetRenderDrawColor(renderer, 40, 40, 255, 255);
//        SDL_RenderDrawLine(renderer, i * (800 / size), 600, i * (800 / size), 600 - height);
//    }
//
//    SDL_RenderDrawLine(renderer, 0, 600, 800, 600);
//
//    SDL_RenderPresent(renderer);
//    SDL_Delay(5000);  // Display for 5 seconds
//
//    SDL_DestroyRenderer(renderer);
//    SDL_DestroyWindow(window);
//    SDL_Quit();
//}



void create_spectrum_window() {

   SDL_Init(SDL_INIT_VIDEO);
   screen = SDL_CreateRGBSurface(0, 800, 600, 32, 0, 0, 0, 0);
   
   // Create a window to display the surface
   window = SDL_CreateWindow("FFT Spectrum (dB)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);

   // Create a window surface
   window_surface = SDL_GetWindowSurface(window);

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        printf("TTF could not initialize! TTF_Error: %s\n", TTF_GetError());
    }

    // Load a font
    font = TTF_OpenFont("/usr/share/fonts/truetype/freefont/FreeMono.ttf", FONTSIZE);
    if (!font) {
        printf("Font could not be loaded! TTF_Error: %s\n", TTF_GetError());
    }

    IMG_Init(IMG_INIT_PNG);
}



void plot_spectrum(double *spectrum, int size) {
 
    // Fill surface with black color for initial clean background
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 30, 0));
//
//    // Find the maximum magnitude in the spectrum for scaling
//    double max_magnitude = -150.0; // Initialize to minimum expected dB
//    for (int i = 0; i < size; i++) {
//        if (spectrum[i] > max_magnitude) {
//            max_magnitude = spectrum[i];
//        }
//    }

    // Scaling factor to map dB to pixel height
    //double scale_range = max_magnitude + 150;  // To map to a visible range
    int dynamic = 150; // dB
    double scaling_factor = 600.0 / dynamic; // Scale height to window's height  

    int height_min[800], height_max[800];
    for (int a = 0; a < 800; a++) {
        height_min[a]=0;
        height_max[a]=0;
    }

    for (int i = 0; i < size; i++) {
        // Normalize dB values to the height range
        double normalized_height = (spectrum[i] + dynamic) * scaling_factor; // Shift by dynamic dB for visibility
        int height = (int)normalized_height;

        int x = (i * 800.0) / size; // Map index to x-coordinate

        if (height < 0) height = 0;  // Ensure height doesn't drop below 0
        if (height > 599) height = 599; // Limit height to window's height

        if (height > height_max[x]) height_max[x] = height;
        if ((height_min[x] == 0) || (height < height_min[x])) height_min[x] = height;
    }

    Uint32 color = SDL_MapRGB(screen->format, 150, 150, 255); // Spectrum color

    for (int x = 0; x < 800; x++) {
        // Debug print lines
        //printf("Index: %d, dB Value: %.2f, Height: %d\n", i, spectrum[i], height);

        // Draw the frequency bin on the surface
        if (height_min[x] == height_max[x]) 
            height_min[x] = 0;       
        for(int y = 599 - height_max[x]; y < 599 - height_min[x]; y++) {
            if (x >= 0 && x < 400) {  // Also ensure x is within bounds            
                ((Uint32*)screen->pixels)[y * screen->w + 400 + x] = color; // Set pixel color
            }
            else {
                ((Uint32*)screen->pixels)[y * screen->w + x - 400] = color; // Set pixel color
            }                           
        }

//        for (int y = 600; y > 600 - height; y--) {
//            if (y >= 0 && y < 600) {  // Ensure we're within bounds
//                int x = (float)(i * 800) / size; // Map index to x-coordinate
//                if (x >= 0 && x < 400) {  // Also ensure x is within bounds
//                    Uint32 color = SDL_MapRGB(screen->format, 255, 255, 255); // White color
//                    ((Uint32*)screen->pixels)[y * screen->w + 400 + x] = color; // Set pixel color
//                }
//                else {
//                    Uint32 color = SDL_MapRGB(screen->format, 255, 255, 255); // White color
//                    ((Uint32*)screen->pixels)[y * screen->w + x - 400] = color; // Set pixel color
//                }
//            }
//        }
    }

    color = SDL_MapRGB(screen->format, 255, 100, 100); // Grid color
    SDL_Color textColor = {255, 255, 255, 255}; // White color
    SDL_Rect dstrect;
    char leveltext[8];

    dstrect.x = 5;

    // Draw grid:
    for(int lvl = 1; lvl < dynamic/10; lvl++) {   // 10 dB grid
        int y = lvl*600/(dynamic/10);
        for(int x = 0; x < 800; x+=2) {
            ((Uint32*)screen->pixels)[y * screen->w + x] = color;
            } 
        sprintf(leveltext,"%d dB", -10*lvl);
        textSurface = TTF_RenderText_Solid(font, leveltext, textColor);
        if (!textSurface) {
            printf("Text surface could not be created! TTF_Error: %s\n", TTF_GetError());
        }
        dstrect.y = y-FONTSIZE-1;
        SDL_BlitSurface(textSurface, NULL, screen, &dstrect);
    } 
    for(int x = 800/10; x < 800; x+=800/10) {
        for(int y = 0; y < 600; y+=2) {
            ((Uint32*)screen->pixels)[y * screen->w + x] = color;
        } 
    } 
    SDL_BlitSurface(screen, NULL, window_surface, NULL);
    SDL_UpdateWindowSurface(window);

    SDL_Delay(5);  // Display for 5 seconds
    
//    SDL_Event PingStop;
//    while (SDL_PollEvent(&PingStop)) {} 

}


#include <tonc.h>
#include <stdlib.h>
#include <string.h>
#include "bytes.h"

extern OBJ_ATTR obj_buffer[128];  // Reference the global buffer
extern bool byte_updated;
extern bool pipe_passed;

unsigned char game_byte = 0;  // Initialize to 0
unsigned char random_byte = 0;
const char* current_gate = NULL;


// Array of logic gates
const char* logic_gates[] = {"AND","OR","XOR","NOT"};

// Static flag to ensure font is loaded only once
static bool font_initialized = false;

// ------------------------------------------------------------------
// Generate a random byte (8 bits)
// ------------------------------------------------------------------
// const int letter_tiles[26] = {0,0,0,0,0,0,7,8,9,0,10,0,0,0,11,0,0,12,0,13,0,0,0,0,0,0};  // A=7, N=8, D=9, X=10, O=11, R=12, T=13
const int letter_tiles[26] = {7,0,0,9,0,0,0,0,0,0,0,0,0,8,11,0,0,12,0,13,0,0,0,10,0,0};

unsigned char generate_next_byte(void)
{
    unsigned char result = 0;
    for(int i=0;i<8;i++)
    {
        result <<= 1;
        result |= rand() & 1;
    }
    return result;
}

// ------------------------------------------------------------------
// Pick a random logic gate
// ------------------------------------------------------------------
const char* generate_next_flag(void)
{
    // int count = sizeof(logic_gates)/sizeof(logic_gates[0]);
    return logic_gates[rand() % 4];
}

// ------------------------------------------------------------------
// Initialize font tiles for OBJ (called once)
// ------------------------------------------------------------------
static void init_font(void)
{
    if (!font_initialized)
    {
        // Load simple font tiles into OBJ VRAM (tiles 5 and 6, after ball/pipes)
        const u32 font_tiles[9][8] = {
            {0x00011000, 0x00111100, 0x01100110, 0x01100110, 0x01100110, 0x01100110, 0x00111100, 0x00011000},   // 0
            {0x00100000, 0x00111000, 0x00110100, 0x00110000, 0x00110000, 0x00110000, 0x00110000, 0x11111100},   // 1
            {0x00011000, 0x01100110, 0x01100110, 0x01111110, 0x01100110, 0x01100110, 0x01100110, 0x01100110},   // A 
            {0x11000011, 0x11000111, 0x11001111, 0x11011011, 0x11110011, 0x11100011, 0x11000011, 0x11000011},   // N ito
            {0x00011111, 0x00111111, 0x01100011, 0x11000011, 0x11000011, 0x01100011, 0x00111111, 0x00011111},   // D ito
            {0x11000011, 0x01100110, 0x01100110, 0x00011000, 0x00011000, 0x01100110, 0x01100110, 0x11000011},   // X
            {0x01111110, 0x01111110, 0x01100110, 0x01100110, 0x01100110, 0x01100110, 0x01111110, 0x01111110},   // O
            {0x11111111, 0x11000011, 0x11000011, 0x00111111, 0x00011111, 0x01110011, 0x11100011, 0x11000011},   // R ito
            {0x01111110, 0x01111110, 0x00011000, 0x00011000, 0x00011000, 0x00011000, 0x00011000, 0x00011000},   // T
        };
        // memcpy(&tile_mem_obj[0][5], font_tiles[0], 32);  // '0' at OBJ tile 5
        // memcpy(&tile_mem_obj[0][6], font_tiles[1], 32);  // '1' at OBJ tile 6
        for(int i=0; i<9; i++)
            memcpy(&tile_mem_obj[0][5+i], font_tiles[i], 32);
        font_initialized = true;
    }
}

void draw_gate(const char* gate, int y)
{
    if (!gate) return;
    init_font();
    clear_gate();
    
    int len = strlen(gate);
    int positions[3] = {106, 116, 126};  // Default for 3-letter gates
    if (strcmp(gate, "OR") == 0) {
        positions[0] = 109;
        positions[1] = 123;
        len = 2;  // Only 2 letters
    }
    
    for(int i=0; i<len; i++)
    {
        char letter = gate[i];
        int tile_id = (letter >= 'A' && letter <= 'Z') ? letter_tiles[letter - 'A'] : 5;  // Default to '0' if invalid
        int obj_id = 33 + i;  // Slots 33-35 for letters
        OBJ_ATTR *obj = &obj_buffer[obj_id];
        
        obj_set_attr(obj,
            ATTR0_SQUARE | ATTR0_4BPP | (y & 0xFF),
            ATTR1_SIZE_8 | (positions[i] & 0x1FF),
            ATTR2_BUILD(tile_id, 0, 0)
        );
    }
}

void clear_gate(void)
{
    for(int i=0; i<3; i++)
    {
        int obj_id = 33 + i;
        obj_buffer[obj_id].attr0 = ATTR0_HIDE;
    }
}


// ------------------------------------------------------------------
// Draw a byte as 8-bit OBJ sprites at exact pixel location
// pixel_x, pixel_y = top-left of first bit
// ------------------------------------------------------------------
// ------------------------------------------------------------------
void draw_byte_bits(unsigned char value, int pixel_x, int pixel_y, int base_obj_id)
{
    init_font();  // Ensure font is loaded
    for(int i=0; i<8; i++)
    {
        int bit = (value & (1 << (7-i))) ? 1 : 0;
        int tile_id = bit ? 6 : 5;  // Tile 5 for '0', 6 for '1'

        int obj_id;
        if (base_obj_id != -1) {
            // Use dynamic base for pipes
            obj_id = base_obj_id + i;
        } else {
            // Original logic: left byte (30) uses 17-24, right byte (150) uses 25-32
            int base_id = (pixel_x == 30) ? 17 : 25;
            obj_id = base_id + i;
        }
        OBJ_ATTR *obj = &obj_buffer[obj_id];

        obj_set_attr(obj,
            ATTR0_SQUARE | ATTR0_4BPP | (pixel_y & 0xFF),          // 8x8, 4BPP, Y position
            ATTR1_SIZE_8 | ((pixel_x + i*8) & 0x1FF),             // X position (8 pixels per bit)
            ATTR2_BUILD(tile_id, 0, 0)  // Tile 5 or 6, palette 0 (ball's colors)
        );
    }
}

// ------------------------------------------------------------------
// Clear previous byte sprites (hide them)
// ------------------------------------------------------------------
void clear_byte_bits(int pixel_x, int pixel_y, int base_obj_id)
{
    int base_id;
    if (base_obj_id != -1) {
        // Use dynamic base for pipes
        base_id = base_obj_id;
    } else {
        // Original logic
        base_id = (pixel_x == 30) ? 17 : 25;
    }
    for(int i=0; i<8; i++)
    {
        int obj_id = base_id + i;
        obj_buffer[obj_id].attr0 = ATTR0_HIDE;  // Hide the sprite
        if (pixel_x == 150 || base_obj_id != -1) clear_gate();  // Only clear gate for right byte or pipes
    }
}


void byte_logic(void)
{
    // Clear previous text sprites
    // clear_byte_bits(30, 12);
    // clear_byte_bits(150, 12);
    clear_byte_bits(150, 12, -1);

    random_byte = generate_next_byte();
    current_gate = generate_next_flag();
    

    // Apply the logic to update game_byte
    if (strcmp(current_gate, "NOT") != 0) {
        draw_byte_bits(random_byte, 150, 12, -1);
        draw_gate(current_gate, 12);
    } else {
        draw_gate(current_gate, 12);  // Only gate for NOT
    }
}

void apply_byte_logic(void)
{
    if (!current_gate) return;
    
    // Apply logic to game_byte
    if (strcmp(current_gate, "NOT") == 0) {
        game_byte = ~game_byte;
    } else {
        if (strcmp(current_gate, "AND") == 0){
            game_byte &= random_byte;
        } else if (strcmp(current_gate, "OR") == 0) {
            game_byte |= random_byte;
        } else if (strcmp(current_gate, "XOR") == 0) {
            game_byte ^= random_byte;
        }
    }
    draw_byte_bits(game_byte, 30, 12, -1);
    // byte_logic();
}
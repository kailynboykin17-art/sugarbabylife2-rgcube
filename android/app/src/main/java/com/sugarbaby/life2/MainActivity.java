package com.sugarbaby.life2;

import org.libsdl.app.SDLActivity;

// Sugarbaby Life 2 for the RG Cube: SDL runs the game (libmain.so)
public class MainActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() { return new String[] { "SDL2", "main" }; }
}

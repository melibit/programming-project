THIS WRITE UP IS INCREDIBLY OUT OF DATE

Please note; the following this is a work-in-progress project for school.

# Computer Science Programming Project Report

## Analysis

### Outline
First-person, DOOM (1993) style game.

#### Case Study

The source-code for [DOOM (1993)](https://github.com/id-Software/DOOM) and its predecessor [Wolfenstien 3D (1992)](https://github.com/id-Software/wolf3d) has been made public by developer ID software; additionally a "Game Engine Black Book" documenting the DOOM engine has been written by [Fabien Sanglard](https://fabiensanglard.net) about [DOOM](https://fabiensanglard.net/gebbdoom) and made freely available; this will prove a useful reference for understanding complex implementation choices and optimisations made by the original DOOM (however I will try not to be overly-reliant on the existing literature and develop the project using my own ideas).


There are two key simplifications of true 3D rendering which DOOM's rendering engine uses which I am replicating:
- It is not truly 3D; the world can be represented entirely from a "birds-eye" perspective, with a series of sectors each with a floor and ceiling height;
- Sectors are convex; and all walls in a sector are the same height.

#### Sucess Criteria

1. DOOM-style renderer;
2. Load levels from file;
3. Movement & collision with walls, floors & objects;
4. Interactable objects;
5. Texture-mapped walls, floors & objects.

#### Inputs & Outputs

##### Inputs

- Movement;
- Camera controls; 
- Level Selection.

##### Outputs 

- Rendered view from camera's perspective
- Debug log

## Design 

My original ideas for the render loop and general architecture; it is of-course simplified slightly, but provides a valuable structure which I can build on.

### Flow-chart 
[asciiflow](https://asciiflow.com/)
```

              Simplified Render Loop                       
         ┌─────────────────────────────┐                   
         │sector_queue <- camera.sector│                   
         └─────────────┬───────────────┘                   
                       │                                   
                       │                                   
                       ▼                                   
       ┌─────────────────────────────────┐                 
       │walls <- sector_queue.next_sector│                 
       └───────────────┬─────────────────┘                 
         ▲             │                                   
         │             │                                   
         │             ▼                                   
         │ N  ┌─────────────────┐                          
         └────┤n < walls.n_walls│◄────────────────────────┐
              └────────┬────────┘                         │
                       │                                  │
                       │Y                                 │
                       ▼                                  │
               ┌────────────────┐                         │
               │wall <- walls[n]│                         │
               └───────┬────────┘                         │
                       │                                  │
                       │                                  │
                       ▼                                  │
              ┌─────────────────┐                         │
    ┌─────────┤? wall.viewportal├─────────────┐           │
    │     N   └─────────────────┘   Y         │           │
    │                                         │           │
    │                                         │           │
    ▼                                         ▼           │
┌───────────────┐ ┌───────────────────────────────┐       │
│draw_wall(wall)│ │sector_queue <- wall.viewportal│       │
└──────┬────────┘ └─────────────────┬─────────────┘       │
       │                            │                     │
       │                            │                     │
       │            ┌──────┐        │                     │
       └───────────►│ n++  │◄───────┘                     │
                    └──┬───┘                              │
                       └──────────────────────────────────┘
```

### Hierarchy Diagram
```
(planned and over-simplified. subject to change.)
                                             ┌──────┐                                   
                                             │      │                                   
                                             │ Main │                                   
                                             │      │                                   
                                             └──┬───┘                                   
                                                │                                       
                        ┌────────────────┬──────┴─────────┬─────────────────┐           
                   ┌────┴─────┐    ┌─────┴─────┐  ┌───────┴────────┐ ┌──────┴──────────┐
                   │          │    │           │  │                │ │                 │
                   │ render() │    │ present() │  │ load_sectors() │ │ process_input() │
                   │          │    │           │  │                │ │                 │
                   └────┬─────┘    └───────────┘  └────────────────┘ └─────────────────┘
                        │                                                               
               ┌────────┴────────┐                                                      
               │                 │                                                      
               │                 │                                                      
        ┌──────┴──────┐ ┌────────┴──────┐                                               
        │             │ │               │                                               
        │ draw_wall() │ │ draw_object() │                                               
        │             │ │               │                                               
        └──────┬──────┘ └──────┬──────┬─┘                                               
               │               │      │                                                 
       ┌───────┴───────────┐   │      │                                                 
       │                   │   │      │                                                 
┌──────┴──────┐  ┌─────────┴───┴────┐ │                                                 
│             │  │                  │ │                                                 
│ draw_line() │  │ sample_texture() │ │                                                 
│             │  │                  │ │                                                 
└──────┬──────┘  └──────────────────┘ │                                                 
       └───────────┐                  │                                                 
                   │                  │                                                 
           ┌───────┴──────┐           │                                                 
           │              │           │                                                 
           │ draw_pixel() │───────────┘                                                 
           │              │                                                             
           └──────────────┘                                                             
```

## Development

I wanted to make the project portable and light-weight, I also wanted to work closely with the graphics and rendering programming, without using a dedicated game engine, therefore, I decided to use the [SDL3](https://wiki.libsdl.org/SDL3/FrontPage) library, as it is cross-platform, relatively easy to build, well-documented and provides a basic framework for rendering a buffer of pixels to the screen and getting user input. I will use no other dependencies.

## Evaluation 

### Success Evaluation

### What I have learnt

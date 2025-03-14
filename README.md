Please note; the following this is a work-in-progress project for school.

# Computer Science Programming Project Report

## Analysis

### Outline
First-person, DOOM (1993) style game.

#### Case Study

The source-code for [DOOM (1993)](https://github.com/id-Software/DOOM) and its predecessor [Wolfenstien 3D (1992)](https://github.com/id-Software/wolf3d) has been made public by developer ID software; additionally a "Game Engine Black Book" documenting the DOOM engine has been written by [Fabien Sanglard](https://fabiensanglard.net) about [DOOM](https://fabiensanglard.net/gebbdoom) and made freely available; this will prove a useful reference for understanding complex implementation choices and optimisations made by the original DOOM (however I will try not to be overly-reliant on the existing literature and develop the project using my own ideas).

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

### Flow-chart 
[asciiflow](https://asciiflow.com/)
```
(planned and over-simplified. subject to change.)

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
                   │ render() │    │ present() │  │ load_sectors() │ │ prodess_input() │
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

## Test Design & Testing 

project progress is slow so please forgive the bad testing. its very hard to test something that doesnt exist yet! 

| Test  | Data | Expected    | Pass/Fail | Feedback  | 
| ----- | ---- | ----------- | --------- | --------- |
| Level Loading | Simple Test Level  | Sectors load Correctly | | |
| "           " | Complex Test Level | Sectors load Correctly | | |
| Rendering | Single Sector Test | Sector renders Correctly | | |
| Map | Single Sector Test | Map Draws Correctly | Pass | Test multi-sectors | 
----
### Test Screenshots:
![lettuce][./lettuce.bmp]


## Development

### Program Code 
```

```

## Evaluation 

### Success Evaluation

### What I have learnt

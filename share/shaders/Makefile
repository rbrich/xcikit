all:    rectangle.vert.spv rectangle.frag.spv \
        sprite.vert.spv sprite.frag.spv \
        sprite_c.vert.spv sprite_c.frag.spv

%.spv: % ; glslc $< -o $@

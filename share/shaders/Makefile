all:    rectangle.vert.spv rectangle.frag.spv \
        sprite.vert.spv sprite.frag.spv \
        sprite_c.vert.spv sprite_c.frag.spv \
        line.vert.spv line.frag.spv \
        ellipse.vert.spv ellipse.frag.spv \
        fps.vert.spv fps.frag.spv \
        cursor.vert.spv cursor.frag.spv

%.spv: % ; glslc $< -o $@

.PHONY: clean
clean:
	rm *.spv

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP | CLK_FILTER_NEAREST;

__kernel void renderSliceToTexture(
        __read_only image3d_t volume,
        __write_only image2d_t texture,
        __private int sliceNr
        ) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int z = sliceNr;

    unsigned char value = read_imageui(volume, sampler, (int4)(x,y,z,0)).x;
    //printf("value: %d\n", value);
    write_imagef(texture, (int2)(get_global_size(0)-x-1,get_global_size(1)-y-1), (float4)(0.0,1.0,0.0,1.0));
}

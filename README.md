# NLA Challenge1

- Use 256px image as input, print related matrix munually for checking, and output the genrated image.

- all the matrices are by rowmajor order and normalized to $[0,1]$.

- vector.mtx represents v.

- sharpen.mtx represents matrix result after sharpening filter. As we can see, there are some values which are greater than 1 or less than 0. We can clip them to $[0,1]$ at converting to `Matrix<unsigned char>` step.

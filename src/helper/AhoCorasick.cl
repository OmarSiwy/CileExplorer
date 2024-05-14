__kernel void Match(__globabl const char* pattern, __global const char* haystack,__global int* location, __global int patternlength, __global int haystacklength) {
  int id = get_global_id(0);
  
  if (location != nullptr) return;
}

__kernel void Match(__global const char* pattern, 
                    __global const char* haystack,
                    __global int* locations, 
                    const int patternlength, 
                    const int haystacklength) {
    int id = get_global_id(0);
    
    // Check if the work item is within bounds
    if (id > haystacklength - patternlength) return;
    
    // Loop through the pattern and compare it with the haystack
    int match = 1;
    for (int i = 0; i < patternlength; i++) {
        if (pattern[i] != haystack[id + i]) {
            match = 0;
            break;
        }
    }
    
    // If a match is found, store the position in the locations array
    if (match == 1) {
        locations[id] = id;
    } else {
        locations[id] = -1;
    }
}

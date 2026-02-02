# JonssonicDSP

JonssonicDSP is a modular, approachable realtime C++ audio DSP library for developers exploring audio programming and building audio effects and tools.

## Library Guidelines

JonssonicDSP is developed with the following principles:

- **Header-only and templated classes:** All core components are implemented as header-only, using C++ templates for flexibility and inlining.
- **Scalar-only processing:** For now, all processing is scalar (single-sample) to maximize code readability and learning value. Vectorized or SIMD processing may be considered in the future.
- **Readable, modular code:** The codebase prioritizes clarity and modularity over micro-optimizations, making it approachable for those learning audio DSP.
- **Minimal dependencies:** The library is self-contained and avoids external dependencies where possible.

For detailed design conventions and API guidelines, see [docs/GUIDELINES.md](docs/GUIDELINES.md).

These guidelines may evolve as the library grows.

## Features 

- Core DSP including audio buffers, delay lines, digital filters, waveshapers, dynamic processors, oversampler, noise generators and LFOs
- Reusable Models build from the core dsp classes, including reverb algortihms, virtual-analog models
- Example audio effects: Delay, Flanger, Reverb, EQ, Compressor, and Distortion
- Multi-channel audio processing support 
- Modular architecture designed for easy extension and integration  
- CMake-based build system for cross-platform development  

---

## Performance

JonssonicDSP prioritizes clarity, modularity, and ease of use. Performance optimizations may be limited as the project focuses on learning and flexibility rather than maximum efficiency.

---

## Getting Started

### Installation

JonssonicDSP is a header-only library. Simply copy the `include/jonssonic/` folder into your project and include the headers you need.

**Using CMake (Recommended):**

```cmake
# Add as subdirectory
add_subdirectory(external/JonssonicDSP)

# Link to your target
target_link_libraries(YourTarget PRIVATE jonssonic::dsp)
```

**Manual Integration:**

```cpp
#include <jonssonic/core/filters/biquad_filter.h>
#include <jonssonic/effects/delay.h>
// etc.
```

### Quick Example

```cpp
#include <jonssonic/core/filters/biquad_filter.h>

int main() {
    // Create a biquad filter object
    jnsc::BiquadFilter<float> filter;
    
    // Prepare for processing
    filter.prepare(44100.0, 1);                     // 44.1kHz, mono

    // Set parameters
    filter.setType(BiquadType::Lowpass); 
    filter.setFreq(Frequency<T>::Hertz(1000.0));    // 1kHz cutoff
    filter.setQ(0.707);                             // Butterworth
    
    // Process audio
    float inputSample = 0.5f;
    float outputSample = filter.processSample(0,inputSample); // process 0th channel
    
    return 0;
}
```

### Documentation

Full API documentation is available at: **[ion3rik.github.io/JonssonicDSP](https://ion3rik.github.io/JonssonicDSP)**

---

## Contributing

Contributions and feedback welcome!

---

## License

MIT License.

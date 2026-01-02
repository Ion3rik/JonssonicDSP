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

Instructions for integration and usage will be added soon.

---

## Contributing

Contributions and feedback welcome!

---

## License

MIT License.

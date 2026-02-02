  # JonssonicDSP Library Guidelines

This document outlines the general conventions, design patterns, and usage guidelines for the JonssonicDSP library. These guidelines aim at ensuring consistency, maintainability, and ease of use for contributors and users. 

## General Principles
- **Header-Only:** The library is designed to be header-only for easy integration.
- **Templated Classes:** All major DSP components are implemented as C++ templates. This provides:
  - Flexibility in data types (e.g., float, double)
  - Customization of algorithmic behavior (e.g., different interpolators, smoothing types, filter orders)
  - Compile-time optimization and inlining
  
  Only structural or algorithmic parameters (such as data type, filter order, or interpolation method) should be template parameters. Runtime parameters (such as smoothing time, cutoff frequency, or gain) must always be set via member functions, not as template arguments. This keeps the API flexible and avoids code bloat from excessive template instantiations.

   - **Global Configuration:** Build-time options (e.g., versioning, max channels, sample rate limits) are set in CMakeLists.txt and reflected in jonssonic_config.h. Refer to these files to customize library-wide settings.
  
 **Modular Structure:** The code is organized in a pyramid of main layers:
   - [core/](../include/Jonssonic/core/) — backbone DSP classes (filters, delays, dynamics, etc.)
   - [models/](../include/Jonssonic/models/) — behavioral models built by combining core classes (e.g., virtual-analog models, reverb algorithms).
   - [effects/](../include/Jonssonic/effects/) — complete audio effects built from models and core classes.
   - [utils/](../include/Jonssonic/utils/) — helper functions and utilities.

 Each layer is further divided into modules by DSP domain or behavioural function e.g.,
 - **core**: Core DSP classes are directly in the `jnsc::` namespace (e.g., `jnsc::BiquadFilter`, `jnsc::DelayLine`, `jnsc::GainComputer`)
 - **models**: `jnsc::models::` namespace (e.g., `jnsc::models::FeedbackDelayNetwork`)
 - **effects**: `jnsc::effects::` namespace (e.g., `jnsc::effects::Reverb`, `jnsc::effects::Flanger`)
 - **utils**: `jnsc::utils::` namespace for helper functions

 The structure is hierarchical:
   - **Effects** depend on models, core, and utils
   - **Models** depend on core and utils
   - **Core** depends only on core and utils
   - **Utils** is standalone and can be used anywhere

 This ensures a clear separation of concerns and minimizes circular dependencies. 
  

- **Public/Internal API:**
  - Public headers are exposed in the `include/` directory and are intended for library users.
  - Internal implementation details are placed in `detail/` subfolders (and namespace) at the module level (e.g., `core/filters/detail/`) to discourage direct use and avoid polluting the public API.
  - All `detail/` folders are excluded from installation and distribution by the build system (e.g., CMake).
  - Module-specific limits (e.g., buffer sizes, algorithm parameters) are defined in a `module_limits.h` file inside the relevant `detail/` subfolder. These are for internal use only and are not exposed to the public API.
  - For larger modules with many enum classes, it is good practice to place them in a specific types file, such as `filter_types.h`, for clarity and maintainability. These types should be part of the public API.
  - Module-specific limits (e.g., buffer sizes, algorithm parameters) are defined in files named after the module, such as `filter_limits.h`, inside the relevant `detail/` subfolder. These are for internal use only and are not exposed to the public API.
  

## Class and API Conventions
 **Processor Classes:**
    - All processor classes provide internal multichannel support.
    - Processor classes are not thread-safe unless otherwise documented.
    - All processor classes provide a `prepare()` method which takes care of all memory allocation. All othere methods should be kept real-time safe.
    - Runtime parameters (e.g., smoothing time, cutoff frequency) are set via dedicated setter methods, not in `prepare()`.
    - All processor classes provide a `reset()` method to clear internal state.
    - All processor classes provide a `processSample()` and when appropriate `processBlock()`, for sample by sample and block processing, respectively.
- **No Copy/Move:** Most processor classes delete copy and move constructors/assignment to avoid accidental state sharing.

## Naming and Style
- **Namespaces:**
  - Public API uses the `jnsc::` namespace for core classes, with `jnsc::models::`, `jnsc::effects::`, and `jnsc::utils::` for higher-level components.
  - Internal helpers and implementation details are always in `detail` namespace (e.g., `jnsc::detail`) 

- **Type Names:**
  - Use `snake_case` for folder and file names.
  - Use `camelCase` for variable and function names.
  - Use `PascalCase` for class and type names.


## Documentation
- **Doxygen:** All public classes, methods, and parameters are documented with Doxygen comments.
- **General Guidelines:** This file (GUIDELINES.md) documents non-class-specific patterns and best practices.

## Error Handling
- **Assertions:** Use `assert` for internal invariants and precondition checks.
- **Clamping:** All parameter setters clamp values to safe, documented ranges.

## Extensibility
- **Template Specialization:** Use template specializations for algorithmic variations (e.g., different smoothing types).

## Testing
- **Test Files:**
  - Test files may use `using namespace` for brevity.
  - Tests should ideally cover all public API and edge cases.

---


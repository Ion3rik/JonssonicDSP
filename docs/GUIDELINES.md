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
   - [models/](../include/Jonssonic/models/) — behavioral models built by combining core classes (e.g., virtual-analog models, reverb algorithms)
   - [effects/](../include/Jonssonic/effects/) — complete audio effects built from models and core classes
   - [utils/](../include/Jonssonic/utils/) — helper functions and utilities

 Each layer is further divided into modules by DSP domain or behavioural function e.g.,
 - **core**: `core::filters`, `core::delays`, `core::dynamics`, etc.
 - **models**: `models::filters`, `models::reverb`, etc.
 - **effects**: `effects::chorus`, `effects::flanger`, etc.

 The structure is hierarchical:
   - **Effects** depend on models, core, and utils
   - **Models** depend on core and utils
   - **Core** depends only on core and utils
   - **Utils** is standalone and can be used anywhere

 This ensures a clear separation of concerns and minimizes circular dependencies. 
  
  The folder structure and C++ namespaces are designed to mirror each other for clarity and maintainability. For example, code in `core/filters/` uses the `jonssonic::core::filters` namespace.

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
- **Type Aliases:** Type aliases for template-dependent types are defined inside each class for clarity and maintainability.

## Naming and Style
- **Namespaces:**
  - Public API uses clear, domain-based namespaces (e.g., `jonssonic::core::filters`, `jonssonic::models::filters`).
  - Internal helpers and utilities are always in `detail` namespace 
- **Type Names:**
  - Use `snake_case` for function and variable names.
  - Use `PascalCase` for class and type names.

- **Enums:**
  - Common enums (e.g., `FilterType`, `EnvelopeType`) are centralized in `*_types.h` files for reuse.

## Documentation
- **Doxygen:** All public classes, methods, and parameters are documented with Doxygen comments.
- **General Guidelines:** This file (GUIDELINES.md) documents non-class-specific patterns and best practices.

## Error Handling
- **Assertions:** Use `assert` for internal invariants and precondition checks.
- **Clamping:** All parameter setters clamp values to safe, documented ranges.

## Extensibility
- **Template Specialization:** Use template specializations for algorithmic variations (e.g., different smoothing types).
- **Future-Proofing:** Use type aliases and clear namespaces to make future refactoring and expansion easy.

## Testing
- **Test Files:**
  - Test files may use `using namespace` for brevity.
  - Tests should ideally cover all public API and edge cases.

---


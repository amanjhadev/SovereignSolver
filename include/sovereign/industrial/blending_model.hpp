#ifndef SOVEREIGN_INDUSTRIAL_BLENDING_MODEL_HPP
#define SOVEREIGN_INDUSTRIAL_BLENDING_MODEL_HPP

#include "sovereign/milp/milp_model.hpp"
#include <string>
#include <vector>

namespace sovereign::industrial {

struct BlendComponent {
    std::string name;
    Float64 cost_per_unit{0.0};
    Float64 min_available{0.0};
    Float64 max_available{1e20};
    std::vector<Float64> property_values; // Property value per unit volume/mass
};

struct BlendSpecification {
    std::string property_name;
    Float64 min_spec{-1e20};
    Float64 max_spec{1e20};
};

class BlendingModelBuilder {
public:
    explicit BlendingModelBuilder(std::string name, Float64 target_volume = 100.0);

    void add_component(const BlendComponent& component);
    void add_specification(const BlendSpecification& spec);

    milp::MILPModel build();

    // Built-in benchmark helper: Haverly Pooling Problem Case 1
    static milp::MILPModel create_haverly_case1();

private:
    std::string name_;
    Float64 target_volume_;
    std::vector<BlendComponent> components_;
    std::vector<BlendSpecification> specs_;
};

} // namespace sovereign::industrial

#endif // SOVEREIGN_INDUSTRIAL_BLENDING_MODEL_HPP

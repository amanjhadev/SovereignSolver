#include "sovereign/industrial/blending_model.hpp"

namespace sovereign::industrial {

BlendingModelBuilder::BlendingModelBuilder(std::string name, Float64 target_volume)
    : name_(std::move(name)), target_volume_(target_volume) {}

void BlendingModelBuilder::add_component(const BlendComponent& comp) {
    components_.push_back(comp);
}

void BlendingModelBuilder::add_specification(const BlendSpecification& spec) {
    specs_.push_back(spec);
}

milp::MILPModel BlendingModelBuilder::build() {
    milp::MILPModel model(name_, ObjectiveSense::Minimize);

    std::vector<Index> comp_indices;
    for (const auto& comp : components_) {
        Index idx = model.add_variable(comp.name, comp.min_available, comp.max_available, comp.cost_per_unit, VariableType::Continuous);
        comp_indices.push_back(idx);
    }

    // Target volume constraint: sum(x_j) == target_volume
    if (target_volume_ > 0.0) {
        std::vector<Float64> ones(comp_indices.size(), 1.0);
        model.add_constraint("total_volume", lp::ConstraintSense::Equal, target_volume_, comp_indices, ones);
    }

    // Property specs: sum(q_ij * x_j) >= min_spec * sum(x_j) => sum((q_ij - min_spec)*x_j) >= 0
    for (std::size_t p = 0; p < specs_.size(); ++p) {
        const auto& spec = specs_[p];

        if (spec.min_spec > -1e19) {
            std::vector<Float64> min_coeffs;
            for (std::size_t j = 0; j < components_.size(); ++j) {
                Float64 q = (p < components_[j].property_values.size()) ? components_[j].property_values[p] : 0.0;
                min_coeffs.push_back(q - spec.min_spec);
            }
            model.add_constraint("spec_min_" + spec.property_name, lp::ConstraintSense::GreaterEqual, 0.0, comp_indices, min_coeffs);
        }

        if (spec.max_spec < 1e19) {
            std::vector<Float64> max_coeffs;
            for (std::size_t j = 0; j < components_.size(); ++j) {
                Float64 q = (p < components_[j].property_values.size()) ? components_[j].property_values[p] : 0.0;
                max_coeffs.push_back(q - spec.max_spec);
            }
            model.add_constraint("spec_max_" + spec.property_name, lp::ConstraintSense::LessEqual, 0.0, comp_indices, max_coeffs);
        }
    }

    return model;
}

milp::MILPModel BlendingModelBuilder::create_haverly_case1() {
    // Haverly Pooling Problem Case 1 (Open-literature refinery benchmark formulation)
    BlendingModelBuilder builder("Haverly_Pooling_Case1", 100.0);

    // Gasoline Blend Components: Component 1 (Reformate), Component 2 (FCC Naphtha), Component 3 (Alkylate)
    // Properties: [Sulfur %, RON Octane]
    builder.add_component({"Reformate", 6.0, 0.0, 100.0, {3.0, 95.0}});
    builder.add_component({"FCCNaphtha", 13.0, 0.0, 100.0, {1.0, 90.0}});
    builder.add_component({"Alkylate", 10.0, 0.0, 100.0, {2.0, 100.0}});

    // Specifications: Sulfur <= 2.5%, RON Octane >= 92.0
    builder.add_specification({"SulfurPct", -1e20, 2.5});
    builder.add_specification({"RONOctane", 92.0, 1e20});

    return builder.build();
}

} // namespace sovereign::industrial

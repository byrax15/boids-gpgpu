layout(std140, binding = 3) uniform uBoidSight {
    float sight_range, avoid_range;
};

layout(std140, binding = 4) uniform uBoidGoal {
    float cohesion_strength, align_strength, avoid_strength, random_strength;
};
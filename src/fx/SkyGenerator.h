#pragma once

class SkyGenerator {
public:
    // presetType: 0 = Dawn, 1 = Noon, 2 = Starry Night
    static unsigned int createProceduralCubemap(int presetType);
};

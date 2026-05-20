#include "SHealth.h"

#include <cstdio>

namespace {
constexpr size_t kPreviewUserIdCount = 10;

void printCategoryDistribution(const CategoryRatios& distribution, int decadeLabel = -1) {
    if (decadeLabel >= 0) {
        printf("%d - ", decadeLabel);
    }

    bool first = true;
    for (BmiCategory category : BmiCategoryMeta::allCategories()) {
        if (!first) {
            printf(", ");
        }
        first = false;
        printf("%s = %f", BmiCategoryMeta::categoryName(category),
               distribution[static_cast<size_t>(categoryIndex(category))]);
    }
    printf("\n");
}

void printDecadeStatistics(const SHealth& shealth, int decade) {
    printCategoryDistribution(shealth.getDecadeDistribution(decade), decade);
}

void printOverallStatistics(const SHealth& shealth) {
    printf("\n[Overall BMI distribution]\n");
    const CategoryRatios distribution = shealth.getOverallDistribution();
    for (BmiCategory category : BmiCategoryMeta::allCategories()) {
        printf("  %s = %f%%\n", BmiCategoryMeta::categoryName(category),
               distribution[static_cast<size_t>(categoryIndex(category))]);
    }
}

void printNormalBmiUsers(const SHealth& shealth) {
    const std::vector<int> userIds = shealth.getNormalBmiUserIds();
    printf("\n[Normal BMI users] count = %zu\n", userIds.size());
    const size_t previewCount =
        userIds.size() < kPreviewUserIdCount ? userIds.size() : kPreviewUserIdCount;
    for (size_t i = 0; i < previewCount; ++i) {
        printf("  id = %d\n", userIds[i]);
    }
    if (userIds.size() > previewCount) {
        printf("  ... (%zu more)\n", userIds.size() - previewCount);
    }
}
}  // namespace

int main() {
    SHealth shealth;
    if (shealth.processFile("shealth.dat") < 0) {
        return 1;
    }

    printf("[Age-decade BMI distribution]\n");
    for (int decade = SHealth::kMinAgeDecade; decade <= SHealth::kMaxAgeDecade;
         decade += SHealth::kAgeDecadeStep) {
        printDecadeStatistics(shealth, decade);
    }

    printOverallStatistics(shealth);
    printNormalBmiUsers(shealth);

    return 0;
}

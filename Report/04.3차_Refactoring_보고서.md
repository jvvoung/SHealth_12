# SHealth BMI 프로젝트 — 3차 리팩토링 보고서

| 항목 | 내용 |
|------|------|
| **작성일** | 2026-05-19 |
| **대상** | SHealth_12 (C++ / CMake / Google Test) |
| **범위** | 잔여 코드 스멜 제거 (네이밍·매직넘버·중복·함수 추출) |
| **선행 작업** | [2차_Refactoring_보고서.md](./2차_Refactoring_보고서.md) |
| **상태** | 3차 완료 |

---

## 1. 개요

본 보고서는 **2차 리팩토링** 이후 수행한 코드 재점검(코드 스멜 탐지) 결과를 바탕으로, **잔여 Minor~Major 스멜**을 제거한 **3차 리팩토링** 내용을 정리한 문서입니다.

2차까지 SRP 분리·기능 추가는 완료되었으나, 다음 항목이 남아 있었습니다.

| 잔여 스멜 | 심각도 |
|----------|--------|
| `DataImputer` 체중/키 보정 로직 **거의 동일한 이중 구현** | Major |
| type 코드 `100~400` **이중 정의** (`SHealth.h` vs `BmiTypes.cpp`) | Major |
| `categoryFromTypeCode` 잘못된 code → `Underweight` 반환 | Major |
| `AgeDecade::kCategoryCount` 등 **네이밍·소속 불일치** | Minor |
| `100.0` (cm→m, %) 등 **분산된 매직넘버** | Minor |
| CSV 컬럼 인덱스 `0~3` 하드코딩 | Minor |
| `BmiStatistics` 연령대 6회 + 전체 1회 **중복 순회** | Minor |
| `SHealthBMI` 출력 로직 **부분 중복** | Minor |

3차에서는 위 항목을 **우선순위 1~5**에 맞춰 일괄 정리했습니다.

---

## 2. 3차 목표 및 달성

| # | 목표 | 수행 |
|---|------|------|
| 1 | `DataImputer` 보정 로직 단일 함수화 | ✅ |
| 2 | type 코드 `LegacyTypeCode` **단일 정의** | ✅ |
| 3 | `categoryFromTypeCode` → `std::optional` + invalid 시 0.0 | ✅ |
| 4 | 물리·통계·결측 상수 네임스페이스화 | ✅ |
| 5 | CSV `Column` enum, `BmiCategoryMeta` 정리 | ✅ |
| 6 | `BmiStatistics` 단일 패스 집계 | ✅ |
| 7 | `SHealthBMI` 출력 함수 통합 | ✅ |
| 8 | 단위 테스트 추가 (`InvalidTypeCodeReturnsZero`) | ✅ |

---

## 3. 상세 변경 내역

### 3.1 `DataImputer` — DRY / 함수 추출 (Major)

**Before:** `imputeWeightsByAgeDecade` / `imputeHeightsByAgeDecade` (~50줄 중복)

**After:** 멤버 포인터 기반 단일 함수

```cpp
void imputeFieldByAgeDecade(std::vector<HealthRecord>& records, FieldMember field);

void DataImputer::imputeMissingValues(std::vector<HealthRecord>& records) {
    imputeFieldByAgeDecade(records, &HealthRecord::weightKg);
    imputeFieldByAgeDecade(records, &HealthRecord::heightCm);
}
```

- 결측 판별: `PhysicalUnits::kMissingValue` (0.0) 사용
- 체중·키 보정 **동일 알고리즘** 유지, 유지보수 지점 1곳으로 축소

---

### 3.2 상수·네이밍 통합 (`BmiTypes.h`)

2차 이후 분산·중복되던 상수를 **역할별 namespace**로 재배치했습니다.

| Namespace | 상수·API | 용도 |
|-----------|----------|------|
| `BmiCategoryMeta` | `kCount`, `allCategories()`, `categoryName()` | 분류 개수·라벨 (구 `AgeDecade::kCategoryCount` 대체) |
| `LegacyTypeCode` | `kUnderweight(100)` … `kObesity(400)` | 레거시 type 코드 **유일 정의** |
| `PhysicalUnits` | `kCentimetersPerMeter`, `kMissingValue`, `kInvalidBmi` | 단위·결측·무효 BMI |
| `StatisticsScale` | `kPercent` | 비율(%) 환산 |
| `AgeDecade` | `kMin/kMax/kStep/kCount` | 연령대만 담당 |

**타입 별칭 변경**

```cpp
// Before
using CategoryRatios = std::array<double, AgeDecade::kCategoryCount>;

// After
using CategoryRatios = std::array<double, BmiCategoryMeta::kCount>;
```

**`SHealth` 레거시 alias**

```cpp
static constexpr int kTypeUnderweight = LegacyTypeCode::kUnderweight;
// … (200, 300, 400 동일 패턴)
```

---

### 3.3 `categoryFromTypeCode` — 오류 처리 (Major)

**Before**

```cpp
BmiCategory categoryFromTypeCode(int typeCode) {
    switch (typeCode) { /* 100~400 */ default: return Underweight; }
}
```

**After**

```cpp
std::optional<BmiCategory> categoryFromTypeCode(int typeCode);
// 알 수 없는 code → std::nullopt

double SHealth::getBmiRatio(int ageDecade, int typeCode) const {
    const std::optional<BmiCategory> category = categoryFromTypeCode(typeCode);
    if (!category.has_value()) {
        return 0.0;
    }
    return getCategoryRatio(ageDecade, category.value());
}
```

- 잘못된 type을 **저체중으로 오분류**하던 버그 은폐 제거
- TC: `InvalidTypeCodeReturnsZero`

---

### 3.4 `CsvReader` — CSV 컬럼 enum (Minor)

```cpp
enum class Column { Id = 0, Age = 1, Weight = 2, Height = 3 };
static constexpr size_t kColumnCount = 4;
```

- `tokens[0~3]` 매직 인덱스 제거
- `tokens.size() < kColumnCount` 검증

---

### 5.5 `BmiCalculator` / `BmiStatistics` — 매직넘버·중복 순회

| 파일 | 변경 |
|------|------|
| `BmiCalculator.cpp` | `heightCm / PhysicalUnits::kCentimetersPerMeter` |
| `BmiStatistics.cpp` | `* StatisticsScale::kPercent / total` |
| `BmiStatistics.cpp` | `record.bmi <= PhysicalUnits::kInvalidBmi` skip |

**집계 단일 패스 (Minor → 개선)**

```
Before: 연령대 for(6) × 전체 records + overall for(1) × records  → 최대 7회 순회
After:  records 1회 순회 → decade + overall 동시 카운트
```

```cpp
for (const HealthRecord& record : records) {
    // overall 집계
    const int decadeIdx = AgeDecade::indexForDecade(AgeDecade::startForAge(record.age));
    // 해당 decade 집계 (유효 연령대만)
}
```

---

### 3.6 `SHealthBMI.cpp` — 출력 통합 (Minor)

| Before | After |
|--------|-------|
| `categoryName` switch (로컬) | `BmiCategoryMeta::categoryName()` |
| `printDecadeStatistics` 4칸 수동 printf | `printCategoryDistribution()` 공통 |
| 미리보기 `10` 하드코딩 | `kPreviewUserIdCount = 10` |

연령대 출력 예:

```
20 - underweight = 3.51, normal = 23.80, overweight = 11.83, obesity = 60.86
```

---

## 4. 해결된 코드 스멜 체크리스트

| ID | 스멜 (2차 후 점검) | 3차 조치 |
|----|-------------------|----------|
| D1/F1 | `DataImputer` 이중 구현 | `imputeFieldByAgeDecade` 통합 |
| M1 | type 코드 이중 정의 | `LegacyTypeCode` 단일화 |
| N3 | invalid type → Underweight | `std::optional` + 0.0 반환 |
| N1 | `AgeDecade::kCategoryCount` | `BmiCategoryMeta::kCount` |
| M2/M3 | 100.0 cm/% | `PhysicalUnits`, `StatisticsScale` |
| M4 | CSV 인덱스 0~3 | `CsvReader::Column` |
| M5 | preview 10 | `kPreviewUserIdCount` |
| M6 | 0.0 결측 암시 | `kMissingValue`, `kInvalidBmi` |
| D2 | 통계 이중 순회 | 단일 패스 `compute` |
| D5 | categoryName switch 중복 | `BmiCategoryMeta::categoryName` |

**Critical** 수준 스멜은 1~2차에서 이미 해소되었으며, 3차 재점검에서 **신규 Critical 미발견**.

---

## 5. Before / After 비교

### 5.1 코드 규모 (대표 파일)

| 파일 | 2차 후 | 3차 후 | 비고 |
|------|--------|--------|------|
| `DataImputer.cpp` | 61행 (중복) | 42행 | −31% |
| `BmiTypes.h` | 42행 | 66행 | 상수·optional API 증가 |
| `BmiStatistics.cpp` | 92행 | 88행 | 단일 패스, 로직 단순화 |
| `SHealthBMI.cpp` | 70행 | 68행 | 출력 통합 |

### 5.2 품질 지표

| 항목 | 2차 후 | 3차 후 |
|------|--------|--------|
| 보정 로직 중복 | 2함수 | **1함수** |
| type 코드 정의 위치 | 2곳 | **1곳** |
| invalid type 처리 | 오분류 가능 | **nullopt / 0.0** |
| 통계 full scan 횟수 | ~7회 | **1회** |
| 단위 테스트 | 9건 | **10건** |

---

## 6. 단위 테스트

### 6.1 전체 목록 (10건)

| 테스트 | 3차 관련 |
|--------|----------|
| `ComputeBmi` | `PhysicalUnits` 간접 검증 |
| `ClassifyBmiBoundaries` | — |
| `ImputeMissingWeightByDecade` | 통합 `imputeField` 경유 |
| `ImputeMissingHeightByDecade` | 통합 `imputeField` 경유 |
| `DecadeDistributionMatchesCategoryRatio` | 단일 패스 후 일관성 |
| `OverallCategoryRatio` | — |
| `NormalBmiUserIds` | — |
| `LegacyTypeCodeCompatibility` | `LegacyTypeCode::kUnderweight` |
| **`InvalidTypeCodeReturnsZero`** | **3차 신규** |
| `SkipsInvalidRows` | `kColumnCount` |

### 6.2 실행 결과

```
100% tests passed, 0 tests failed out of 10
Total Test time (real) ≈ 0.26 sec
```

---

## 7. 리팩토링 단계별 누적 요약

| 단계 | 초점 | 핵심 성과 |
|------|------|----------|
| **1차** | 클린코드 | God Method 분해, 버그 수정, 기본 TC |
| **2차** | SRP·기능 | 클래스 분리, height 보정, 전체 비율, 정상 ID 목록 |
| **3차** | 잔여 스멜 | DRY·상수 통합·optional·단일 패스·출력 통합 |

### 7.1 현재 아키텍처 (3차 완료 시점)

```
SHealthBMI (main)
    └── SHealth (파사드)
            ├── CsvReader      … Column enum, kColumnCount
            ├── DataImputer    … imputeFieldByAgeDecade (단일)
            ├── BmiCalculator  … PhysicalUnits
            ├── BmiStatistics  … 단일 패스, StatisticsScale
            └── BmiTypes       … LegacyTypeCode, BmiCategoryMeta, optional
```

---

## 8. 잔여·향후 과제

3차 이후에도 **선택적**으로 개선 가능한 항목입니다.

| 항목 | 설명 |
|------|------|
| `SHealth` static 위임 | `computeBmi` 등 1줄 위임 — 레거시 호환용, 제거 시 breaking change |
| `getBmiRatio` vs `getCategoryRatio` | 레거시 API 병존 — 문서화 후 deprecated 표기 가능 |
| CLI 인자 | `shealth.dat` 경로 하드코딩 |
| `shealth.dat` 통합 TC | 실제 데이터 회귀 테스트 |
| 3차 보고서 외 | 발표용 Before/After 1·2·3차 통합 슬라이드 |

---

## 9. 회고 (3차)

### 9.1 달성

- 2차 직후 코드 스멜 재점검에서 지적한 **Major 3건을 모두 해소**했다.
- 상수·메타데이터를 `BmiTypes` 중심으로 모아 **의미 단위가 명확**해졌다.
- 통계·보정 핫패스의 **불필요한 반복 순회를 줄였다**.

### 9.2 설계 원칙

- **단일 진실 공급원(Single Source of Truth)**: type 코드, 분류 개수, 라벨 문자열
- **명시적 실패**: optional로 invalid 입력 구분
- **제네릭보다 단순함**: C++ 멤버 포인터로 보정 통합 (과도한 템플릿 지양)

### 9.3 한계

- 기능 추가 없이 **내부 품질**만 개선한 단계이므로, 사용자 가시 출력 형식은 2차와 동일하다.
- `PhysicalUnits::kMissingValue == 0.0` 가정은 도메인 규칙에 묶여 있어, 다른 결측 표현 도입 시 추가 설계 필요.

---

## 10. 부록 — 주요 API (3차)

```cpp
// 레거시 type (단일 정의)
LegacyTypeCode::kUnderweight  // 100

// 분류 조회 (권장)
shealth.getCategoryRatio(20, BmiCategory::Normal);

// 레거시 (invalid type → 0.0)
shealth.getBmiRatio(20, 999);

// optional 변환
std::optional<BmiCategory> cat = categoryFromTypeCode(200);

// 분류 메타
for (BmiCategory c : BmiCategoryMeta::allCategories()) { ... }
```

---

*본 문서는 3차 리팩토링(잔여 코드 스멜 제거) 작업 결과를 바탕으로 작성되었습니다.*  
*1차: [1차_Refactoring_보고서.md](./1차_Refactoring_보고서.md) · 2차: [2차_Refactoring_보고서.md](./2차_Refactoring_보고서.md)*

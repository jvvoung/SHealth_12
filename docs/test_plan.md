# SHealth BMI — 테스트 계획서

| 항목 | 내용 |
|------|------|
| **작성일** | 2026-05-20 |
| **브랜치** | `tc` (TC 보강) |
| **기준 문서** | [README.md](../README.md), [requirements_analysis.md](./requirements_analysis.md) |
| **대상** | `shealth_lib` + `SHealthBMITest` (Google Test, ctest) |
| **관점** | 시니어 QA 리드 — BMI 경계·연령대·결측 보정·통계 API |

---

## 0. 처리 파이프라인 (테스트 관점)

```
CsvReader::load
    → DataImputer::imputeMissingValues  (weight → height 순)
    → BmiCalculator::applyToAll
    → BmiStatistics::compute
         ↑ SHealth::processFile 고정 순서
```

**QA 핵심 불변식**

1. 결측 판별: `== 0.0` (epsilon 아님)
2. `heightCm ≤ 0` → `bmi = 0` (`kInvalidBmi`)
3. `bmi ≤ 0` → 통계·ID 목록 **제외** (분류는 Underweight 가능)
4. 연령대 집계: `AgeDecade::contains` + `indexForDecade` 유효 시만
5. 전체 집계: 유효 BMI만; 20 미만·80+ 연령도 **overall**에는 포함 가능

---

## 1) 테스트 범위·우선순위

### 1.1 모듈별 단위 테스트 대상

| 모듈 | 공개 API·로직 | 단위 직접 테스트 | 통합(`SHealth`) | 비고 |
|------|---------------|------------------|-----------------|------|
| **BmiCalculator** | `compute`, `classify`, `applyToAll` | ✅ 권장 | ✅ 기존 | 순수 함수·경계 집중 |
| **DataImputer** | `imputeMissingValues` | ✅ 권장 | ✅ 기존 | weight→height 순서 검증 |
| **CsvReader** | `load`, `split`(private) | ✅ 권장 | ✅ 일부 | 파일 I/O·skip 규칙 |
| **BmiStatistics** | `compute`, 비율·ID API | ✅ 권장 | ✅ 기존 | 분모 0·단일 패스 |
| **BmiTypes** | `AgeDecade::*`, `categoryFromTypeCode`, `categoryName` | ✅ 권장 | 부분 | 메타·레거시 매핑 |
| **SHealth** | `processFile`, 파사드 위임 | 통합 중심 | ✅ | 실패 경로(-1) |
| **SHealthBMI.cpp** | `main`, CLI | P2 (선택) | — | E2E·수동 |

### 1.2 우선순위 정의

| 등급 | 의미 | 게이트 |
|------|------|--------|
| **P0** | 도메인 정확성·크래시 방지·파이프라인 불변식 | **Green 필수**, PR 차단 |
| **P1** | 경계·엣지·회귀 방지 | 1차 스프린트 내 완료 권장 |
| **P2** | 골든 데이터·CLI·커버리지 90%+ | 선택·CI 강화 시 |

### 1.3 P0 / P1 / P2 시나리오 표

| ID | 시나리오 | 모듈 | 등급 | 기존 TC | 상태 |
|----|----------|------|------|---------|------|
| TC-01 | BMI 공식 (정상 height) | BmiCalculator | P0 | `ComputeBmi` | ✅ 커버 |
| TC-02 | BMI 경계 18.5 / 18.5001 / 23.0 / 25.0 | BmiCalculator | P0 | `ClassifyBmiBoundaries` | ✅ 커버 |
| TC-03 | height≤0 → bmi=0 | BmiCalculator | P0 | `ComputeBmi` | ✅ 커버 |
| TC-04 | 체중 0 → 연령대 평균 보정 | DataImputer+통합 | P0 | `ImputeMissingWeightByDecade` | ✅ 커버 |
| TC-05 | 키 0 → 연령대 평균 보정 | DataImputer | P0 | `ImputeMissingHeightByDecade` | ✅ 커버 |
| TC-06 | 연령대 분포 ≡ getCategoryRatio | BmiStatistics+SHealth | P0 | `DecadeDistributionMatchesCategoryRatio` | ✅ 커버 |
| TC-07 | 전체 4분류 합 ≈ 100% | BmiStatistics | P0 | `OverallCategoryRatio` | ✅ 커버 |
| TC-08 | 정상 BMI ID 1건 | BmiStatistics | P0 | `NormalBmiUserIds` | ✅ 커버 (단일) |
| TC-09 | LegacyTypeCode 100~400 | BmiTypes+SHealth | P0 | `LegacyTypeCodeCompatibility` | ✅ 커버 |
| TC-10 | invalid type → 0.0 / nullopt | BmiTypes+SHealth | P0 | `InvalidTypeCodeReturnsZero` | ✅ 커버 |
| TC-11 | CSV 빈 줄·bad-row skip | CsvReader | P0 | `SkipsInvalidRows` | ✅ 커버 |
| TC-12 | processFile 파일 없음 → -1 | SHealth+CsvReader | P0 | — | ❌ **추가 필요** |
| TC-13 | BMI 22.99 Normal / 24.99 Overweight | BmiCalculator | P1 | — | ⚠️ **보강** |
| TC-14 | BMI 0·음수 → 통계 제외 | BmiStatistics | P1 | — | ❌ **추가 필요** |
| TC-15 | age 19·71·80 — decade vs overall | AgeDecade+통계 | P1 | — | ❌ **추가 필요** |
| TC-16 | 동 대 전원 weight=0 — 보정 실패 | DataImputer | P1 | — | ❌ **추가 필요** |
| TC-17 | 체중·키 동시 0 파이프라인 | 통합 | P1 | — | ❌ **추가 필요** |
| TC-18 | getNormalBmiUserIds 0건·복수 | BmiStatistics | P1 | — | ⚠️ **보강** |
| TC-19 | 빈 연령대 비율 0 / invalid decade | BmiStatistics | P1 | — | ⚠️ **보강** |
| TC-20 | categoryFromTypeCode 100/200/300/400 각각 | BmiTypes | P1 | — | ⚠️ **보강** |
| TC-21 | CSV 헤더만 / 컬럼 3개 / 파싱 실패 | CsvReader | P1 | 부분 | ⚠️ **보강** |
| TC-22 | 단일 레코드 overall 합 100% | BmiStatistics | P1 | — | ❌ **추가 필요** |
| TC-23 | height 음수·극소값(≠0) | BmiCalculator | P1 | — | ❌ **추가 필요** |
| TC-24 | weight 음수 보정 전 compute | BmiCalculator | P2 | — | ❌ 추가 |
| TC-25 | `shealth.dat` 골든 회귀 | 통합 | P2 | — | ❌ 추가 |
| TC-26 | `categoryName` 전 분기 | BmiTypes | P2 | — | ❌ 추가 |
| TC-27 | `AgeDecade::indexForDecade` invalid | BmiTypes | P2 | — | ❌ 추가 |

### 1.4 TEST_F 도입 권장 여부

| Fixture | 용도 | 도입 | 근거 |
|---------|------|------|------|
| **CsvProcessFixture** | `writeTempCsv` + `SHealth::processFile` + `TearDown`에서 temp 삭제 | **권장 (P1)** | 7/10 TC가 동일 보일러플레이트; 경로·정리 일원화 |
| **RecordsPipelineFixture** | `vector<HealthRecord>` + impute + applyToAll + statistics | **권장 (P1)** | `ImputeMissingHeightByDecade` 패턴 재사용 |
| **BmiCalculatorFixture** | 없음 | **비권장** | static API만, `TEST`로 충분 |
| **Parametrize (TEST_P)** | BMI 경계·typeCode 매트릭스 | **선택 (P1)** | `ClassifyBmiBoundaries` 확장 시 가독성↑ |

**결론:** 현재 10건 `TEST` 유지 가능. **TC 12건 이상**부터 `CsvProcessFixture` 도입 시 ROI 높음. Fixture는 **공유 mutable 상태 금지** (테스트 간 파일명 충돌 방지: `tmp_path` 또는 고유 suffix).

### 1.5 기존 10건 TC 매핑

| # | 테스트명 | 직접 검증 모듈 | requirements § | 커버 | 미커버·보강 |
|---|----------|----------------|------------------|------|-------------|
| 1 | `ComputeBmi` | BmiCalculator, SHealth 위임 | §2.1 | 공식, height=0 | weight≤0, 음수 height |
| 2 | `ClassifyBmiBoundaries` | BmiCalculator | §2.2 매트릭스 | 18.5, 18.5001, 23, 25 | 22.99, 24.99, bmi≤0 분류 |
| 3 | `ImputeMissingWeightByDecade` | 통합 | §4, FI-01 | 체중 보정→비만 100% | validCount=0, 복수 결측 |
| 4 | `ImputeMissingHeightByDecade` | DataImputer | FI-02 | 키 보정·BMI 일치 | CSV 경유 파이프라인 |
| 5 | `DecadeDistributionMatchesCategoryRatio` | BmiStatistics | FI-01 | 20대 2분류 일치 | 빈 대, invalid decade |
| 6 | `OverallCategoryRatio` | BmiStatistics | FI-04 | 3명 합 100% | 1명, 0명, decade별 합 |
| 7 | `NormalBmiUserIds` | BmiStatistics | FI-03 | id 1건 | 0건·복수·bmi≤0 제외 |
| 8 | `LegacyTypeCodeCompatibility` | SHealth | §2.3 | 100 vs Underweight | 200/300/400 개별 |
| 9 | `InvalidTypeCodeReturnsZero` | BmiTypes, SHealth | §2.3 | 999→nullopt, ratio 0 | 경계 code (99, 101) |
| 10 | `SkipsInvalidRows` | CsvReader | §1.2 | 빈 줄, bad-row, 건수 2 | 헤더만, 컬럼 부족, 파일 없음 |

---

## 2) 경계값 케이스 목록

### 2.1 BMI 분류 (`BmiCalculator::classify`)

| 입력 BMI | 기대 `BmiCategory` | 우선순위 | 기존 TC | 상태 |
|----------|-------------------|----------|---------|------|
| 18.5 | Underweight | P0 | ✓ | ✅ |
| 18.5001 | Normal | P0 | ✓ | ✅ |
| 22.999 | Normal | P1 | — | ❌ 추가 |
| 23.0 | Overweight | P0 | ✓ | ✅ |
| 24.999 | Overweight | P1 | — | ❌ 추가 |
| 25.0 | Obesity | P0 | ✓ | ✅ |
| 0.0 | Underweight (분류) / 통계 제외 | P1 | — | ❌ 추가 |
| -1.0 | Underweight (분류) / 통계 제외 | P1 | — | ❌ 추가 |

### 2.2 BMI 계산 (`BmiCalculator::compute`)

| weight (kg) | height (cm) | 기대 bmi | 우선순위 | 기존 TC | 상태 |
|-------------|-------------|----------|----------|---------|------|
| 70 | 175 | ≈22.857 | P0 | ✓ | ✅ |
| 70 | 0 | 0.0 | P0 | ✓ | ✅ |
| 70 | -10 | 0.0 | P1 | — | ❌ 추가 |
| 70 | 0.001 | 매우 큰 값 (≠0 결측) | P2 | — | ❌ (데이터 계약) |
| 0 | 170 | 0 (보정 전) | P1 | — | ❌ 추가 |
| -5 | 170 | 음수 BMI 가능 | P2 | — | ❌ (명세 미정) |

### 2.3 height / weight 결측·보정

| 시나리오 | 기대 | 우선순위 | 기존 TC | 상태 |
|----------|------|----------|---------|------|
| weight=0, 동 대 유효 1+ | 평균 대입 후 BMI 재계산 | P0 | ✓ | ✅ |
| height=0, 동 대 유효 1+ | 평균 cm 대입 | P0 | ✓ | ✅ |
| weight·height 동시 0 | weight 먼저 → height | P1 | — | ❌ 추가 |
| 동 대 전원 weight=0 | 미보정, bmi=0, 통계 0 | P1 | — | ❌ 추가 |
| 동 대 전원 height=0 (weight 유효) | height 미보정 | P1 | — | ❌ 추가 |
| 보정 후 재계산 | `applyToAll` 후 bmi>0 | P0 | 간접 | ⚠️ 명시 TC 권장 |

### 2.4 age·연령대 (`AgeDecade`)

| age | `startForAge` | `contains(age, 20)` | decade 집계 | overall 집계 | 우선순위 | 상태 |
|-----|---------------|---------------------|-------------|--------------|----------|------|
| 19 | 10 | false | 제외 | 유효 BMI면 포함 | P1 | ❌ |
| 20 | 20 | true (20대) | 포함 | 포함 | P1 | ❌ |
| 29 | 20 | true | 포함 | 포함 | P1 | ❌ |
| 30 | 30 | false@20, true@30 | 30대 | 포함 | P1 | ❌ |
| 70 | 70 | true (70대) | 포함 | 포함 | P1 | ❌ |
| 71 | 70 | true (70≤71<80) | **70대** | 포함 | P1 | ❌ |
| 80 | 80 | index -1 | 제외 | 포함 가능 | P1 | ❌ |

### 2.5 비율·분포 (`BmiStatistics`)

| 시나리오 | 기대 | 우선순위 | 기존 TC | 상태 |
|----------|------|----------|---------|------|
| 연령대 유효 2명, 2분류 | decade 합 ≈ 100% | P0 | 간접 | ⚠️ 합 assert 추가 |
| 전체 3명 | overall 합 ≈ 100% | P0 | ✓ | ✅ |
| 전체 1명 | 해당 범주 100% | P1 | — | ❌ |
| 연령대 0명 (빈 대) | `getCategoryRatio` → 0 | P1 | — | ❌ |
| invalid decade (10, 80) | 0.0 / `{}` | P1 | — | ❌ |
| 분모 0 (전원 bmi≤0) | 전부 0, 크래시 없음 | P1 | — | ❌ |

---

## 3) 예외·특이 케이스 목록

### 3.1 CSV (`CsvReader::load`)

| 시나리오 | 기대 | 우선순위 | 기존 TC | 상태 |
|----------|------|----------|---------|------|
| 파일 없음 | `load` false, `processFile` **-1** | P0 | — | ❌ **추가** |
| 헤더만 (데이터 0행) | `processFile` **0**, 비율 전부 0 | P1 | — | ❌ |
| 빈 줄 | skip | P0 | ✓ | ✅ |
| 컬럼 3개 (`tokens.size()<4`) | skip | P1 | — | ❌ |
| `stoi`/`stod` 실패 | skip | P0 | bad-row | ⚠️ 부분 |
| 잘못된 헤더 문자열 | 1행 폐기 후 파싱 시도 | P2 | — | ❌ (문서화) |
| id 중복 | 미정의 | P2 | — | — |

### 3.2 API·파이프라인

| 시나리오 | 기대 | 우선순위 | 기존 TC | 상태 |
|----------|------|----------|---------|------|
| `processFile` 성공 | `records_.size()` | P0 | 간접 | ✅ |
| `getBmiRatio(..., 999)` | 0.0 | P0 | ✓ | ✅ |
| `categoryFromTypeCode(999)` | `nullopt` | P0 | ✓ | ✅ |
| `categoryFromTypeCode(100~400)` | 각 `optional` 값 | P1 | 간접 | ⚠️ 개별 TC |
| 보정 불가 `validCount==0` | 필드 0 유지 | P1 | — | ❌ |
| `bmi≤0` 통계 제외 | count·ID 미포함 | P1 | — | ❌ |
| `getDecadeDistribution(99)` | `{}` | P1 | — | ❌ |
| `processFile` 전 API 호출 | 0 반환 (미처리 상태) | P2 | — | ❌ |

### 3.3 레거시 호환

| typeCode | `BmiCategory` | `getBmiRatio` ≡ `getCategoryRatio` | 상태 |
|----------|---------------|-------------------------------------|------|
| 100 | Underweight | 동일 | ⚠️ 간접 |
| 200 | Normal | 동일 | ❌ 개별 추가 |
| 300 | Overweight | 동일 | ❌ |
| 400 | Obesity | 동일 | ❌ |
| 99, 500 | — | 0.0 | ❌ |

### 3.4 requirements_analysis 대조 — 구현·TC 종합

| 요구 ID | 내용 | 구현 | TC | 종합 상태 |
|---------|------|------|-----|-----------|
| REQ-BMI-01 | BMI 공식 | ✅ | ✅ | **커버됨** |
| REQ-BMI-02 | 4분류 경계 | ✅ | 부분 | **보강** (22.99, 24.99) |
| REQ-BMI-03 | height≤0 → bmi=0 | ✅ | ✅ | **커버됨** |
| REQ-BMI-04 | bmi≤0 통계 제외 | ✅ | ❌ | **추가 필요** |
| REQ-IMP-01 | weight=0 보정 | ✅ | ✅ | **커버됨** |
| REQ-IMP-02 | height=0 보정 (FI-02) | ✅ | ✅ | **커버됨** |
| REQ-IMP-03 | validCount=0 스킵 | ✅ | ❌ | **추가 필요** |
| REQ-AGE-01 | 20~70, 10년 단위 | ✅ | 간접 | **보강** (19, 71, 80) |
| REQ-AGE-02 | 71~79 → 70대 | ✅ | ❌ | **추가 필요** |
| REQ-AGE-03 | age<20 decade 제외, overall 포함 | ✅ | ❌ | **추가 필요** |
| REQ-STAT-01 | decade 비율 % | ✅ | ✅ | **커버됨** |
| REQ-STAT-02 | overall 합 100% | ✅ | ✅ | **커버됨** |
| REQ-STAT-03 | 분모 0 → 0 | ✅ | ❌ | **추가 필요** |
| REQ-STAT-04 | Normal ID 목록 (FI-03) | ✅ | 부분 | **보강** |
| REQ-CSV-01 | 4열 미만 skip | ✅ | 부분 | **보강** |
| REQ-CSV-02 | 파일 없음 -1 | ✅ | ❌ | **추가 필요** |
| REQ-LEG-01 | type 100~400 | ✅ | 부분 | **보강** |
| REQ-LEG-02 | invalid → 0 | ✅ | ✅ | **커버됨** |
| FI-01 | 연령대 분포 | ✅ | ✅ | **커버됨** |
| FI-02 | height 보정 | ✅ | ✅ | **커버됨** |
| FI-03 | 정상 ID 목록 | ✅ | 부분 | **보강** |
| FI-04 | 전체 비율 | ✅ | ✅ | **커버됨** |

---

## 4) Google Test 설계 가이드

### 4.1 TEST vs TEST_F

| 기준 | `TEST` | `TEST_F` |
|------|--------|----------|
| 상태 | 없음 | Fixture `SetUp`/`TearDown` |
| 적합 | 순수 static, 단일 assert 소수 | CSV 파일·공통 파이프라인 |
| 예 | `ClassifyBmiBoundaries` | `CsvProcessFixture.ProcessReturnsCount` |

**네이밍:** `TEST(Suite, Given_When_Then)` 또는 `TEST_F(Fixture, ShouldExpectedWhenCondition)`  
**주석:** Given / When / Then 3줄 블록 (한국어·영어 혼용 가능)

```cpp
// Given: 20대 1명 유효 체중, 1명 체중 결측
// When:  processFile
// Then:  결측자 BMI는 평균 체중 기준, 20대 Obesity 100%
```

### 4.2 부동소수 비교

| 용도 | 매크로 | 허용 오차 | 예 |
|------|--------|-----------|-----|
| BMI 값 | `EXPECT_NEAR` | **0.01** | 공식 검증 |
| 보정 후 BMI 일치 | `EXPECT_NEAR` | **0.001** | 동일 대화자 비교 |
| 비율(%) | `EXPECT_NEAR` | **0.01** | 합 100% |
| height=0 → bmi | `EXPECT_DOUBLE_EQ` | 0 | 정확히 0.0 |
| enum 분류 | `EXPECT_EQ` | — | `BmiCategory` |

### 4.3 임시 CSV 헬퍼

**현재 패턴** (`SHealthBMITest.cpp`):

```cpp
std::string writeTempCsv(const std::string& content) {
    const std::string path = "test_shealth_temp.csv";  // 고정명 — 병렬 실행 시 충돌 위험
    std::ofstream file(path);
    file << content;
    return path;
}
```

**개선안 (Fixture 통합):**

```cpp
class CsvProcessFixture : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = "test_shealth_" + std::to_string(++counter_) + ".csv";
    }
    void TearDown() override { std::remove(path_.c_str()); }
    std::string writeCsv(const std::string& body) const { /* path_에 기록 */ }
    int process(SHealth& sh, const std::string& body) {
        writeCsv("id,age,weight,height\n" + body);
        return sh.processFile(path_);
    }
    std::string path_;
    static inline int counter_{0};
};
```

- **헤더 포함 여부:** 본문만 넘기면 Fixture가 헤더 prepend
- **실행 디렉터리:** ctest는 `build/`에서 실행 → temp 파일은 `build/` 하위 (`.gitignore` 권장)

### 4.4 신규 TC 목록 (번호·이름·입력·기대)

| ID | 테스트명 (안) | 입력 | 기대값 |
|----|---------------|------|--------|
| TC-11 | `ProcessFile_MissingFile_ReturnsMinusOne` | `no_such_file.csv` | `-1`, records empty |
| TC-12 | `ClassifyBmi_NormalUpperBound_22_999` | `classify(22.999)` | `Normal` |
| TC-13 | `ClassifyBmi_OverweightUpperBound_24_999` | `classify(24.999)` | `Overweight` |
| TC-14 | `Statistics_ExcludesNonPositiveBmi` | records bmi=0, valid=1 | overall count 1 only |
| TC-15 | `AgeDecade_Age19_ExcludedFromDecade_InOverall` | age 19, valid BMI | decade(20)=0, overall>0 |
| TC-16 | `AgeDecade_Age71_InSeventiesBucket` | age 71 | `getCategoryRatio(70,...)` >0 |
| TC-17 | `Impute_AllZeroWeightInDecade_NoImpute` | 20대 전원 weight=0 | weight still 0, ratios 0 |
| TC-18 | `Impute_BothZeroWeightThenHeight` | w=0,h=0, peer valid | both imputed, bmi>0 |
| TC-19 | `NormalBmiUserIds_MultipleAndEmpty` | 0 normal / 2 normal | size 0 / 2, order preserved |
| TC-20 | `Csv_HeaderOnly_ReturnsZero` | header only | `processFile`→0 |
| TC-21 | `Csv_ThreeColumns_Skipped` | `1,25,60` | count 0 |
| TC-22 | `CategoryFromTypeCode_AllLegacyCodes` | 100,200,300,400 | 4× `has_value` + enum |
| TC-23 | `Overall_SingleRecord_OneHundredPercent` | 1 record | one category 100% |
| TC-24 | `ComputeBmi_NegativeHeight_ReturnsZero` | height=-1 | bmi=0 |
| TC-25 | `DecadeDistribution_EmptyDecade_AllZero` | only 30대 data | 20대 sum=0 |

---

## 5) 커버리지 목표·측정·개선 전략

### 5.1 모듈별 목표 (1차 스프린트)

| 모듈 | Line | Branch | Function | 비고 |
|------|------|--------|----------|------|
| **BmiCalculator** | 95%+ | 90%+ | 100% | 소규모·핵심 |
| **BmiTypes** | 90%+ | 85%+ | 90%+ | `categoryName` default |
| **DataImputer** | 90%+ | 85%+ | 100% | `validCount==0` 분기 |
| **CsvReader** | 85%+ | 80%+ | 100% | catch, empty line |
| **BmiStatistics** | 90%+ | 85%+ | 100% | decadeIdx<0, total≤0 |
| **SHealth** | 85%+ | 80%+ | 90%+ | 위임 다수 |
| **SHealthBMI.cpp** | — | — | — | P2 제외 |
| **전체 shealth_lib** | **88%+** | **82%+** | **95%+** | tc 브랜치 목표 |

### 5.2 gcov / lcov CMake 도입

**현재:** `CMakeLists.txt`에 커버리지 플래그 없음.

**Linux / GCC (권장 — CI용):**

```cmake
option(SHEALTH_ENABLE_COVERAGE "Enable gcov coverage" OFF)
if(SHEALTH_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(shealth_lib PRIVATE --coverage -fprofile-arcs -ftest-coverage)
  target_link_options(shealth_lib PUBLIC --coverage)
  target_compile_options(SHealthBMITest PRIVATE --coverage)
  target_link_options(SHealthBMITest PRIVATE --coverage)
endif()
```

```bash
cmake -S . -B build-cov -DSHEALTH_ENABLE_COVERAGE=ON
cmake --build build-cov
cd build-cov && ctest --output-on-failure
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/googletest/*' '*/test/*' --output-file coverage.filtered.info
genhtml coverage.filtered.info --output-directory coverage-html
```

**Windows / MSVC 대안:**

| 도구 | 용도 |
|------|------|
| **OpenCppCoverage** | Visual Studio 빌드 후 HTML 리포트 |
| **llvm-cov** + Clang-cl | `--coverage` (설치 복잡) |
| **WSL2 + GCC** | 위 lcov 플로우와 동일 (팀 표준 권장) |

**실무 권장:** 로컬(Windows)은 `ctest` Green 유지, **커버리지 게이트는 Linux CI** 1개만.

### 5.3 미커버 예상 구간

| 위치 | 이유 | 대응 TC |
|------|------|---------|
| `SHealthBMI.cpp` `main` | 단위 테스트 미대상 | P2 수동/E2E |
| `CsvReader::split` | private, `load` 경유만 | CSV 변형 TC |
| `BmiCategoryMeta::categoryName` default | 도달 어려움 | enum 전체 순회 TC |
| `SHealth::processFile` stderr | 파일 없음 | TC-11 |
| `DataImputer` `validCount==0` continue | 극단 데이터 | TC-17 |
| `BmiStatistics` `decadeIdx<0` continue | age 19, 80 | TC-15 |
| `AgeDecade::indexForDecade` invalid | 10, 80, 15 | TC-25, Parametrize |

### 5.4 ROI 높은 TC (커버리지·리스크)

1. **TC-11** — `processFile` -1 (실패 경로)
2. **TC-17** — 전원 결측 보정 실패
3. **TC-14** — bmi≤0 통계 제외 (분류·집계 불일치 방지)
4. **TC-15 / TC-16** — 연령 경계 (요구사항 갭 최대)
5. **TC-12 / TC-13** — off-by-one 분류
6. **TC-22** — typeCode 4종 개별
7. **TC-20 / TC-21** — CSV 엣지
8. **TC-18** — FI-03 복수 ID
9. **TC-23** — 단일 레코드 overall
10. **TC-25** — 빈 연령대

---

## 6) 실행·CI 체크리스트

### 6.1 로컬 빌드·테스트

```bash
# 구성
mkdir -p build && cd build
cmake ..
cmake --build .

# 테스트 (실패 시 로그)
ctest --output-on-failure

# Windows (Visual Studio Generator)
cmake --build . --config Debug
ctest -C Debug --output-on-failure
```

| 단계 | 명령 | 통과 기준 |
|------|------|-----------|
| Configure | `cmake ..` | 오류 없음 |
| Build | `cmake --build .` | `SHealthBMITest` 생성 |
| Test | `ctest --output-on-failure` | **100% passed** |
| (선택) Coverage | §5.2 | 모듈 목표 충족 |

### 6.2 회귀 게이트 (리팩토링 전후)

| 게이트 | 조건 |
|--------|------|
| **G0** | 기존 10 TC 전부 Green |
| **G1** | P0 신규 TC (TC-11 등) 추가 후 Green |
| **G2** | P1 TC 80% 이상 완료 |
| **G3** | 커버리지 shealth_lib line ≥ 88% (CI) |

**리팩토링 PR 규칙:** 동작 변경 없음 → G0 필수. 동작 변경(명세 정렬) → requirements_analysis 갱신 + 해당 TC 동반.

### 6.3 CI 파이프라인 (권장 초안)

```yaml
# 예: GitHub Actions
- cmake -B build -DCMAKE_BUILD_TYPE=Debug
- cmake --build build
- cd build && ctest --output-on-failure
# (선택) -DSHEALTH_ENABLE_COVERAGE=ON + lcov + artifact upload
```

### 6.4 tc 브랜치 작업 순서

1. **Phase A (P0):** TC-11, 기존 10건 Green 유지  
2. **Phase B (P1):** 경계·연령·결측·CSV — §4.4 TC-12~25  
3. **Phase C:** `CsvProcessFixture` 리팩터, 중복 제거  
4. **Phase D (P2):** 커버리지 CMake, `shealth.dat` 골든, CI  

---

## 부록 A — 테스트 파일 구조 (권장)

```
test/cpp/
  SHealthBMITest.cpp          # 기존 + 신규 TC
  test_helpers/
    TempCsv.h                 # writeTempCsv, Fixture (선택)
  BmiCalculatorTest.cpp       # (선택) 모듈 분리 시
```

## 부록 B — 참조 링크

- [requirements_analysis.md](./requirements_analysis.md) — 경계값 매트릭스·명세 vs 구현
- [README.md](../README.md) — 빌드·현재 TC 목록
- [03.3차_Refactoring_보고서.md](../Report/03.3차_Refactoring_보고서.md) — 아키텍처·API

---

*본 문서는 `tc` 브랜치 TC 보강의 기준 계획서이며, TC 구현 PR마다 §1.3·§1.5 표의 상태 열을 갱신한다.*

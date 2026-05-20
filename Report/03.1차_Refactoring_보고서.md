# SHealth BMI 프로젝트 — 1차 리팩토링 보고서

| 항목 | 내용 |
|------|------|
| **작성일** | 2026-05-19 |
| **대상** | SHealth_12 (C++ / CMake / Google Test) |
| **범위** | 코드 스멜 분석 → 1차 리팩토링 (클린코드) → 기본 단위 테스트 |
| **상태** | 1차 완료, 2차(기능 개선·SRP 분리) 미착수 |

---

## 1. 개요

본 보고서는 삼성 헬스 BMI 통계 C++ 프로젝트에 대해 수행한 **코드 스멜 분석**과 **1차 리팩토링** 결과를 정리한 문서입니다.

### 1.1 프로젝트 목적

- CSV(`id, age, weight, height`)에서 BMI를 계산한다.
- 연령대(10년 단위, 20~70대)별로 저체중·정상·과체중·비만 **비율(%)**을 산출한다.
- **보정 규칙**: `weight == 0`이면 동일 연령대의 평균 체중으로 대체한다.

### 1.2 BMI 분류 기준

| 구분 | 조건 |
|------|------|
| 저체중 | BMI ≤ 18.5 |
| 정상 | 18.5 초과 ~ 23 미만 |
| 과체중 | 23 이상 ~ 25 미만 |
| 비만 | 25 이상 |

### 1.3 대상 파일

```
src/main/cpp/SHealth.h
src/main/cpp/SHealth.cpp
src/main/cpp/SHealthBMI.cpp
src/test/cpp/SHealthBMITest.cpp
```

---

## 2. 초기 코드 스멜 분석 요약

리팩토링 전 코드를 클린코드 관점에서 분석한 주요 이슈는 다음과 같습니다.

### 2.1 구조적 문제

| 스멜 유형 | 설명 |
|----------|------|
| **God Class / SRP 위반** | `SHealth`가 CSV I/O, 파싱, 보정, BMI 계산, 통계, 조회를 모두 담당 |
| **Long Method** | `calculateBmi()` 약 100줄에 전체 파이프라인 집중 |
| **Primitive Obsession** | `ages[]`, `weights[]`, `heights[]`, `bmis[]` 병렬 고정 배열 |
| **Data Clump** | 연령대×분류별 `underweight20` … `obesity70` **24개 멤버 변수** |
| **DRY 위반** | 연령대 루프 3회, `a==20/30/…` 분기 6회, `getBmiRatio` 24분기 if-else |

### 2.2 명명·가독성

| 이슈 | 설명 |
|------|------|
| **Misleading Name** | `calculateBmi`가 BMI만이 아니라 파일 처리·통계까지 수행 |
| **Magic Number** | `18.5`, `23`, `25`, `100/200/300/400`, `10000`, `20~70` 등 산재 |
| **Poor Naming** | `type` 코드(100, 200…)의 의미 불명확 |

### 2.3 버그·안전성 (Critical)

| # | 문제 | 위치(리팩토링 전) |
|---|------|------------------|
| 1 | BMI **25.0**이 `> 25` 조건으로 **미분류** | `SHealth.cpp` 71행 |
| 2 | 연령대 내 유효 체중 없을 때 `sum / ageCount` **0 나누기** | 44행 |
| 3 | `height == 0`일 때 BMI 계산 **0 나누기** | 52행 |
| 4 | 연령대 인원 `sum == 0`일 때 비율 계산 **0 나누기** | 77행 이후 |
| 5 | CSV 컬럼 부족 시 `tokens[1~3]` **범위 초과** | 21~23행 |
| 6 | 빈 줄에서 `break`로 **이후 데이터 무시** | 18~19행 |
| 7 | 테스트 `FAIL()`로 **CI 항상 실패** | `SHealthBMITest.cpp` |

---

## 3. 1차 리팩토링 목표 및 범위

README 실습 계획의 **「2. 1차 리펙토링」** 항목에 맞춰 다음을 수행했습니다.

| 목표 | 수행 여부 |
|------|----------|
| 네이밍 개선 | ✅ |
| 하드코드·전역(고정 배열) 제거 | ✅ |
| 함수 추출 | ✅ |
| 반복/중복 제거 | ✅ |
| 경계값·0 나누기 등 버그 수정 | ✅ |
| 기본 단위 테스트 추가 | ✅ |
| SRP 클래스 분리 (2차) | ⬜ 미수행 |
| height=0 보정, 전체 비율 등 기능 추가 (2차) | ⬜ 미수행 |

---

## 4. 상세 변경 내역

### 4.1 네이밍·API

| Before | After |
|--------|-------|
| `calculateBmi(filename)` | `processFile(filename)` |
| `getBmiRatio(age, 100~400)` | `getCategoryRatio(age, BmiCategory)` + 레거시 `getBmiRatio` 유지 |
| `weights`, `heights` (배열) | `HealthRecord.weightKg`, `heightCm` |

### 4.2 상수·타입 도입 (`SHealth.h`)

```cpp
enum class BmiCategory { Underweight, Normal, Overweight, Obesity };

namespace BmiThresholds {
    constexpr double kUnderweightMax = 18.5;
    constexpr double kNormalMax = 23.0;
    constexpr double kOverweightMax = 25.0;
}

struct HealthRecord {
    int age;
    double weightKg;
    double heightCm;
    double bmi;
};
```

- 연령대 상수: `kMinAgeDecade`, `kMaxAgeDecade`, `kAgeDecadeStep`, `kDecadeCount`
- 레거시 type 코드: `kTypeUnderweight(100)` … `kTypeObesity(400)`

### 4.3 데이터 구조 개선

| Before | After |
|--------|-------|
| `int ages[10000]` 등 4개 고정 배열 | `std::vector<HealthRecord> records_` |
| `underweight20` … `obesity70` (24개) | `std::array<std::array<double, 4>, 6> categoryRatios_` |

### 4.4 함수 분리 (`SHealth.cpp`)

```
processFile()
  ├── loadRecordsFromFile()    // CSV 로드·검증·예외 처리
  ├── imputeMissingWeights()   // 연령대별 weight==0 보정
  ├── computeAllBmis()         // BMI 계산
  └── computeDecadeStatistics() // 연령대별 4분류 비율(%)

[static, 테스트 가능]
  ├── computeBmi(weightKg, heightCm)
  ├── classifyBmi(bmi)
  ├── ageDecadeStart(age)
  └── isInAgeDecade(age, decadeStart)
```

### 4.5 중복 제거

- 연령대별 ratio 저장: `decadeIndex()` + 2차원 배열 인덱싱으로 **6×4 분기 제거**
- `getBmiRatio`: 24분기 → `getCategoryRatio` + `categoryFromTypeCode` 위임
- `SHealthBMI.cpp`: 6회 `printf` → **연령대 루프 1회**

### 4.6 버그·안전성 수정

| 이슈 | 조치 |
|------|------|
| BMI 25.0 미분류 | `classifyBmi`에서 `>= kOverweightMax(25)` → `Obesity` |
| height == 0 | `computeBmi`에서 0 반환, 통계 집계 시 `bmi <= 0` 제외 |
| ageCount == 0 (보정 불가) | 평균 계산·대체 생략 |
| sum == 0 (빈 연령대) | 비율 계산 생략, ratio 0 유지 |
| 잘못된 CSV 행 | `tokens.size() < 4` 또는 파싱 예외 시 **skip** |
| 빈 줄 | `continue` (전체 읽기 중단하지 않음) |
| 파일 오픈 실패 | `processFile` **-1** 반환 (성공 시 건수) |

### 4.7 `SHealthBMI.cpp`

- `processFile` 실패 시 exit code `1`
- `getCategoryRatio` + `BmiCategory` enum 사용
- 연령대 출력을 `kMinAgeDecade` ~ `kMaxAgeDecade` 루프로 통합

---

## 5. Before / After 비교

### 5.1 코드 규모 (대략)

| 파일 | Before | After |
|------|--------|-------|
| `SHealth.h` | 28행 | 58행 |
| `SHealth.cpp` | 148행 | 195행 |
| `SHealthBMI.cpp` | 28행 | 35행 |
| `SHealthBMITest.cpp` | 6행 (`FAIL`) | 72행 (5개 TC) |

> 구현 파일 행 수는 소폭 증가했으나, **단일 거대 함수 → 역할별 함수**로 분리되어 가독성·테스트 가능성이 개선되었습니다.

### 5.2 설계 품질

| 항목 | Before | After |
|------|--------|-------|
| 단일 책임 (함수 단위) | ❌ | ✅ (파이프라인 단계별 분리) |
| 매직 넘버 | 다수 | `BmiThresholds`, `kType*` 등 상수화 |
| BMI 분류 로직 | 4곳 if-else 중복 | `classifyBmi` **한 곳** |
| 단위 테스트 | 불가 (`FAIL`) | static 메서드 + fixture CSV |
| 버퍼 오버플로 위험 | `count` 무제한 증가 | `vector` 동적 확장 |

### 5.3 실행 결과 (`shealth.dat`, 프로젝트 루트 기준)

리팩토링 후 `SHealthBMI` 실행 예시 (BMI=25 경계 수정 반영):

```
20 - underweight = 3.51, normal = 23.80, overweight = 11.83, obesity = 60.86
30 - underweight = 1.86, normal = 15.53, overweight = 10.06, obesity = 72.55
40 - underweight = 0.52, normal = 10.04, overweight = 9.13,  obesity = 80.31
50 - underweight = 2.18, normal = 12.63, overweight = 9.99,  obesity = 75.20
60 - underweight = 0.86, normal = 8.53,  overweight = 10.64, obesity = 79.96
70 - underweight = 0.53, normal = 12.35, overweight = 10.76, obesity = 76.37
```

> 비만 경계 수정으로, 이전 코드 대비 **정확히 BMI 25.0인 사용자**가 비만으로 집계되며 일부 연령대 비만 비율이 소폭 변동할 수 있습니다.

---

## 6. 단위 테스트

### 6.1 테스트 목록

| 테스트 | 검증 내용 |
|--------|----------|
| `ComputeBmi` | BMI 공식, height=0 방어 |
| `ClassifyBmiBoundaries` | 18.5, 23, **25** 경계값 |
| `ImputeMissingWeightByDecade` | weight=0 → 동 연령대 평균 대체 |
| `LegacyTypeCodeCompatibility` | `getBmiRatio(100~400)` 호환 |
| `SkipsInvalidRows` | 빈 줄·잘못된 행 skip |

### 6.2 실행 결과

```
100% tests passed, 0 tests failed out of 5
Total Test time (real) = 0.13 sec
```

### 6.3 빌드·테스트 명령

```powershell
cd c:\DEV\SHealth_12\build
cmake ..
cmake --build .
ctest --output-on-failure

cd c:\DEV\SHealth_12
.\build\Debug\SHealthBMI.exe
```

---

## 7. 해결된 스멜 vs 잔여 이슈

### 7.1 1차에서 해결·완화

- Long Method → `processFile` 오케스트레이션 + 4개 private 함수
- Magic Number → `BmiThresholds`, 연령대/분류 상수
- DRY (24 멤버, 24분기, 6회 printf) → 배열·루프·단일 분류 함수
- Primitive Obsession → `HealthRecord` + `vector`
- 경계값 버그 (BMI 25), 0 나누기, CSV 검증, 빈 줄 처리
- 테스트 부재 → 5개 TC

### 7.2 2차에서 다룰 항목 (미착수)

| 항목 | 설명 |
|------|------|
| **SRP 클래스 분리** | `CsvReader`, `BmiCalculator`, `StatisticsAggregator` 등 |
| **height=0 보정** | README 4단계 요구사항 |
| **전체 사용자 BMI 비율** | 연령대가 아닌 전체 집계 |
| **정상 BMI 사용자 목록** | ID 목록 조회 API |
| **연령대 BMI 분포** | 세분화 통계 기능 |
| **예외 TC 확장** | 대용량 파일, 손상 데이터 비율 등 |
| **입력 경로** | CLI 인자, `shealth.dat` 상대 경로 개선 |

---

## 8. 회고 (1차)

### 8.1 달성

- 분석 단계에서 식별한 **Critical 버그**(BMI 25, 0 나누기)를 우선 수정했다.
- God Class 내부 로직을 **함수 단위로 분리**하여 이후 2차 SRP 분리의 기반을 마련했다.
- `classifyBmi`, `computeBmi`를 static으로 노출해 **테스트 가능한 구조**로 전환했다.

### 8.2 한계

- 여전히 `SHealth` 하나가 로드~통계까지 담당 (클래스 수준 SRP는 2차 과제).
- `height=0`은 계산만 막고 **평균 키 보정은 미구현**.
- 통합 테스트는 임시 CSV 파일(`test_shealth_temp.csv`)에 의존.

### 8.3 권장 다음 단계

1. **2차 리팩토링**: 책임별 클래스 분리 + README 기능 4종 추가  
2. **테스트 보강**: `shealth.dat` 기반 통합 TC, 예외·경계 시나리오 확대  
3. **Before/After 수치 비교**: 경계 수정 전후 obesity 비율 diff 문서화  

---

## 9. 부록 — 주요 API 변경 참고

```cpp
// Before
int count = shealth.calculateBmi("shealth.dat");
double ratio = shealth.getBmiRatio(20, 100);

// After
int count = shealth.processFile("shealth.dat");  // 실패 시 -1
double ratio = shealth.getCategoryRatio(20, BmiCategory::Underweight);
double legacy = shealth.getBmiRatio(20, SHealth::kTypeUnderweight);  // 호환
```

---

*본 문서는 코드 스멜 분석 및 1차 리팩토링 작업 결과를 바탕으로 작성되었습니다.*

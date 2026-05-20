# SHealth BMI — 요구사항 분석 (C++ 구현·테스트 관점)

> **기준 문서**: `README.md`  
> **대조 구현**: `src/main/cpp/`, `src/test/cpp/` (2026-05 기준)  
> **관점**: 시니어 C++ QA — 도메인 경계값·결측 보정·통계 API·테스트 설계

---

## 0. 처리 파이프라인 (공통)

| 단계 | 담당 | 설명 |
|------|------|------|
| 1 | `CsvReader::load` | CSV → `HealthRecord` 벡터 (BMI 미계산) |
| 2 | `DataImputer::imputeMissingValues` | `weightKg==0`, `heightCm==0` 연령대 평균 보정 |
| 3 | `BmiCalculator::applyToAll` | BMI 계산·`record.bmi` 저장 |
| 4 | `BmiStatistics::compute` | 연령대·전체 집계·정상 BMI ID 목록 |

`SHealth::processFile`이 위 순서를 고정 호출한다.

---

## 1) 도메인·데이터 모델

### 1.1 `HealthRecord` 필드

| 필드 | 타입 | 의미 | 단위 | 초기값(구현) | 비고 |
|------|------|------|------|--------------|------|
| `id` | `int` | 사용자 식별자 | 없음 | `0` | CSV 1열 |
| `age` | `int` | 만 나이 | 세 | `0` | CSV 2열; 연령대·보정 키 |
| `weightKg` | `double` | 체중 | kg | `0.0` | `0.0` = 결측(보정 대상) |
| `heightCm` | `double` | 신장 | cm | `0.0` | `0.0` = 결측(보정 대상); BMI 시 m로 변환 |
| `bmi` | `double` | 체질량지수 | kg/m² | `0.0` | 로드 직후 0; 계산 후 저장. `≤0`은 통계에서 **제외** |

**명세 vs 구현**

| 항목 | README 명세 | 현재 구현 | 차이 |
|------|-------------|-----------|------|
| 결측 표현 | 체중 0만 명시 | `weightKg==0`, `heightCm==0` 동일 처리 | README는 체중만; **기능 개선**으로 키 보정 추가·구현 완료 |
| `bmi` 필드 | 미명시 | `HealthRecord` 멤버 | 구현상 명시적 모델 |

### 1.2 CSV 파싱 규칙 (`CsvReader`)

| 규칙 | README 명세 | 현재 구현 | 차이 |
|------|-------------|-----------|------|
| 파일 형식 | `id,age,weight,height` | 동일 (`Column` enum 순서) | 일치 |
| 헤더 | 샘플 1행 `id,age,weight,height` | 첫 줄 `getline` 후 **폐기** (검증 없음) | 헤더 문자열 불일치 시에도 2행부터 파싱 시도 |
| 구분자 | 쉼표 | `,` | 일치 |
| 컬럼 수 | 4열 | `tokens.size() < 4` → **행 skip** | 일치 |
| 빈 줄 | 미명시 | `line.empty()` → **continue** | 구현만 명시 |
| 파싱 실패 | 미명시 | `stoi`/`stod` 예외 → **catch 후 skip** | 구현만 명시 |
| 파일 없음 | 미명시 | `load` → `false`, `processFile` → **-1** | 구현만 명시 |
| `bmi` | — | 로드 시 계산 안 함 | 파이프라인 3단계에서 계산 |

---

## 2) BMI 계산·분류 비즈니스 규칙

### 2.1 BMI 계산 (`BmiCalculator::compute`)

| 항목 | 공식/규칙 | README | 구현 |
|------|-----------|--------|------|
| 공식 | `BMI = weightKg / (heightM)²`, `heightM = heightCm / 100` | 일치 | 일치 |
| `heightCm ≤ 0` | README 미명시 | `heightCm <= 0` → **`bmi = 0`** (`kInvalidBmi`) | 0으로 나누기 방어 |
| `weightKg ≤ 0` (보정 전) | — | 공식 그대로 계산 가능(음수 BMI 가능) | 보정 파이프라인 후 재계산 가정 |
| `bmi ≤ 0` 통계 | — | `BmiStatistics`에서 **전부 제외** | 명세 보강(구현) |

### 2.2 `BmiCategory` 분류 (`BmiCalculator::classify`)

README: *≤18.5 저체중, 18.5초과~23미만 정상, 23이상~25미만 과체중, 25이상 비만*

| 범주 | 조건식 (수학) | 구현 조건 (`BmiThresholds`) | 경계 포함 |
|------|---------------|----------------------------|-----------|
| Underweight (저체중) | BMI **≤ 18.5** | `bmi <= 18.5` | **18.5 포함** (저체중) |
| Normal (정상) | **18.5 <** BMI **< 23** | `bmi < 23` (이전 분기 탈락 후) | 18.5는 저체중; 23.0은 정상 **아님** |
| Overweight (과체중) | **23 ≤** BMI **< 25** | `bmi < 25` (이전 분기 탈락 후) | **23.0 포함** (과체중) |
| Obesity (비만) | BMI **≥ 25** | `else` | **25.0 포함** (비만) |

**경계값 검증 매트릭스 (테스트 필수)**

| BMI | 기대 범주 | 구현 | 테스트 (`SHealthBMITest`) |
|-----|-----------|------|---------------------------|
| 18.5 | Underweight | Underweight | `ClassifyBmiBoundaries` ✓ |
| 18.5001 | Normal | Normal | ✓ |
| 22.999… | Normal | Normal | (간접) |
| 23.0 | Overweight | Overweight | ✓ |
| 24.999… | Overweight | Overweight | — |
| 25.0 | Obesity | Obesity | ✓ |

**명세 vs 구현**

| 항목 | README | 구현 | 차이 |
|------|--------|------|------|
| 25.0 비만 | “25이상” | `Obesity` | 일치 (레거시 코드 버그 수정됨) |
| `bmi ≤ 0` 분류 | 미명시 | `Underweight`로 분류되나 **통계 제외** | QA: 분류·집계 불일치 인지 필요 |

### 2.3 `LegacyTypeCode` ↔ `BmiCategory`

| typeCode | 상수 | BmiCategory | `categoryFromTypeCode` |
|----------|------|-------------|------------------------|
| 100 | `kUnderweight` | Underweight | `optional` 값 |
| 200 | `kNormal` | Normal | |
| 300 | `kOverweight` | Overweight | |
| 400 | `kObesity` | Obesity | |
| 기타 | — | — | `std::nullopt` → `getBmiRatio` **0.0** |

`SHealth::getBmiRatio(ageDecade, typeCode)`는 유효 코드일 때 `getCategoryRatio`와 동일 값 반환.

---

## 3) 연령대(Age Decade) 규칙

### 3.1 구간 정의

상수 (`AgeDecade`): `kMin=20`, `kMax=70`, `kStep=10`, `kCount=6` (20·30·40·50·60·70대 라벨)

| 연령대 라벨 | `decadeStart` | 포함 나이 (`contains`) | README | 구현 |
|-------------|---------------|--------------------------|--------|------|
| 20대 | 20 | **20 ≤ age < 30** | “20대” (10년 단위 암시) | 일치 |
| 30대 | 30 | 30 ≤ age < 40 | | 일치 |
| … | … | … | | |
| 70대 | 70 | **70 ≤ age < 80** | “~70세” 모호 | **71~79세도 70대 버킷** |

`startForAge(age) = (age / 10) * 10` — 통계 인덱스용 시작 연령.

`indexForDecade(decadeStart)`: `decadeStart ∈ {20,30,40,50,60,70}` 만 유효, 그 외 **-1**.

### 3.2 범위 밖 사용자

| 상황 | 연령대 집계 | 전체(overall) 집계 | 명세 vs 구현 |
|------|-------------|-------------------|--------------|
| age < 20 (예: 17세) | **제외** (`indexForDecade` 또는 `contains` 실패) | 유효 BMI면 **포함** | README “20~70”만 언급 → 구현은 전체 집계에 포함 |
| age ≥ 80 | 80대 시작 → 인덱스 무효 → **제외** | 유효 BMI면 **포함** | 동일 |
| age 71~79 | **70대**에 포함 | 포함 | README “70세” 상한 해석 모호 |

### 3.3 연령대 vs 전체 집계

| API | 분모 | 분자 | 무효 BMI |
|-----|------|------|----------|
| `getDecadeDistribution` / `getCategoryRatio` | 해당 `decadeStart`·`contains` 충족 **및** 유효 BMI 사용자 수 | 동 연령대·해당 범주 수 | `bmi <= 0` 제외 |
| `getOverallDistribution` / `getOverallCategoryRatio` | 전 레코드 중 유효 BMI 수 | 전체 범주별 수 | 동일 |
| 합계 | 연령대별 4비율 합 ≈ **100%** (해당 대에 1명 이상일 때) | 전체 4비율 합 ≈ **100%** | 분모 0 → 비율 **0** (`buildRatios`) |

---

## 4) 결측치 보정(Imputation)

### 4.1 트리거·대상

| 필드 | 결측 조건 | README | 구현 |
|------|-----------|--------|------|
| 체중 | `weightKg == 0.0` | “체중 0이 누락” | `== kMissingValue(0.0)` (**부동소수 정확히 0**) |
| 키 | `heightCm == 0.0` | README 본문 없음 | **기능 개선** — 동일 로직 |

### 4.2 보정 순서·알고리즘 (`DataImputer`)

1. **`weightKg`** — `decade = 20, 30, …, 70` 각각:
   - 동 연령대(`contains`)에서 `value != 0`인 레코드로 평균 산출
   - `validCount == 0` → 해당 대 **스킵**(보정 없음)
   - 결측(`==0`) 레코드에 평균 대입
2. **`heightCm`** — 위와 동일 (`imputeFieldByAgeDecade` 재사용)

**파이프라인**: CSV 로드 → **보정** → **BMI 재계산** → 통계.

| 시나리오 | 기대 동작 | 구현 |
|----------|-----------|------|
| 동 대에 유효 체중 1명+ | 결측 체중 = 그 평균 | ✓ |
| 동 대 전원 `weight==0` | 보정 안 됨 → BMI 0 가능 → 통계 제외 | ✓ |
| 체중·키 모두 0 | 체중 먼저 보정 후 키 보정(각각 유효값만으로 평균) | ✓ |
| 보정 후에도 `height==0` | BMI=0, 통계 제외 | ✓ |

**명세 vs 구현**

| 항목 | README | 구현 |
|------|--------|------|
| 키 결측 | 미언급 | 구현·테스트 있음 (`ImputeMissingHeightByDecade`) |
| 보정 시 이미 대입된 값 사용 | 미명시 | 평균 산출 시 **원본 유효값만** (이중 보정 없음) |

---

## 5) 통계·조회 API

### 5.1 `processFile`

| 항목 | 명세(README+관례) | 구현 |
|------|-------------------|------|
| 성공 | 로드된 **유효 행 수** (`records_.size()`) | `static_cast<int>(records_.size())` |
| 실패 | 파일 열기 실패 | **-1**, stderr 메시지 |
| 부분 실패 행 | — | skip된 행은 개수에 **미포함** |

### 5.2 API 요약

| 메서드 | 입력 | 출력 | 단위 | 분모 0 / 무효 |
|--------|------|------|------|----------------|
| `getCategoryRatio(ageDecade, category)` | 20,30,…,70, `BmiCategory` | 해당 대 범주 비율 | **0~100** (%) | 잘못된 decade → **0.0** |
| `getBmiRatio(ageDecade, typeCode)` | + 레거시 100/200/300/400 | 위와 동일 또는 0 | % | invalid code → **0.0** |
| `getDecadeDistribution(ageDecade)` | decade 시작값 | `CategoryRatios[4]` | % 합≈100 | invalid → `{}` |
| `getOverallDistribution()` | — | 4범주 비율 배열 | % | 전체 유효 0명 → 0 |
| `getOverallCategoryRatio(category)` | `BmiCategory` | 단일 범주 전체 비율 | % | 0 |
| `getNormalBmiUserIds()` | — | `vector<int>` id 목록 | — | Normal·유효 BMI만; **입력 순서 유지** |
| `records()` | — | const 참조 | — | — |

`CategoryRatios` 인덱스: `categoryIndex(BmiCategory)` → Underweight=0 … Obesity=3.

---

## 6) C++ 구현·테스트 주의점

| 주제 | 권장 사항 | 현재 코드 상태 |
|------|-----------|----------------|
| 부동소수 BMI | `EXPECT_NEAR(..., 0.01)` 또는 `0.001` | 계산 TC 0.01, 보정 후 BMI 0.001 |
| 경계 18.5/23/25 | `EXPECT_EQ` on `BmiCategory` (정수 enum) | ✓ |
| `height==0` | compute 전·후, 통계 제외 검증 | compute→0 TC 있음 |
| 빈 연령대 / `validCount==0` | 보정·비율 0, 크래시 없음 | ✓ |
| CSV `tokens.size()` | `< 4` skip | ✓ |
| 결측 판별 | `== 0.0` not `epsilon` | 극소값은 결측 아님 — 데이터 계약 |
| `enum class BmiCategory` | 스위치 exhaustiveness, `static_cast` 인덱스 | 테스트에서 명시적 사용 |
| `std::optional` type code | invalid → nullopt | ✓ |
| `constexpr` 임계값 | `BmiThresholds`, `AgeDecade` | 단일 소스 — TC는 상수 변경 시 동반 수정 |

---

## 7) README “기능 개선” 항목 명세

| ID | 기능 | 입력 | 출력 | 전제조건 | 예외·엣지 |
|----|------|------|------|----------|-----------|
| **FI-01** | 특정 연령대 BMI 분포 비율 | `ageDecade` (20,30,…,70) | `getDecadeDistribution` / `getCategoryRatio` — 4범주 % | `processFile` 완료 | invalid decade → 0; 대 내 유효 0명 → 0; README 본문엔 없었으나 **구현·TC 완료** |
| **FI-02** | `height==0` 평균 보정 | `HealthRecord` 벡터(결측 height) | 동 연령대 평균 cm 대입 | 동 대 `height!=0` 1건 이상 | 전원 0 → 미보정; README 체중만 언급 → **구현·TC 완료** |
| **FI-03** | 정상 BMI 사용자 ID 목록 | — | `getNormalBmiUserIds()` | 18.5 < BMI < 23, `bmi>0` | 순서=처리 순; 중복 id 없음(데이터 가정) |
| **FI-04** | 전체 대비 범주별 비율 | `BmiCategory` 또는 전체 배열 | `getOverallCategoryRatio` / `getOverallDistribution` | 유효 BMI ≥1 | 0명 → 0%; 4합≈100% |

**SRP 리팩토링(README 4단계)**: `CsvReader`, `BmiCalculator`, `DataImputer`, `BmiStatistics`, `SHealth` 파사드 — **구현 완료**.

---

## 8) Google Test 시나리오 목록

### 8.1 구현된 TC (`SHealthBMITest.cpp`)

| # | 시나리오 | 테스트명 | 상태 |
|---|----------|----------|------|
| 1 | BMI 공식·height=0→0 | `ComputeBmi` | ✓ |
| 2 | 경계 18.5, 18.5001, 23.0, 25.0 | `ClassifyBmiBoundaries` | ✓ |
| 3 | 체중 0 → 동 20대 평균 보정·비만 100% | `ImputeMissingWeightByDecade` | ✓ |
| 4 | 키 0 → 동 대 평균·BMI 일치 | `ImputeMissingHeightByDecade` | ✓ |
| 5 | `getDecadeDistribution` ≡ `getCategoryRatio` | `DecadeDistributionMatchesCategoryRatio` | ✓ |
| 6 | 전체 분포 합 100%, `getOverallCategoryRatio` | `OverallCategoryRatio` | ✓ |
| 7 | 정상 BMI id 1건 | `NormalBmiUserIds` | ✓ |
| 8 | 레거시 type 100~400 | `LegacyTypeCodeCompatibility` | ✓ |
| 9 | invalid type → 0 | `InvalidTypeCodeReturnsZero` | ✓ |
| 10 | 빈 줄·bad-row skip, 건수 2 | `SkipsInvalidRows` | ✓ |

### 8.2 추가 권장 TC (미구현·보강)

| # | 시나리오 | 목적 |
|---|----------|------|
| 11 | `processFile` 존재하지 않는 파일 → -1 | 실패 경로 |
| 12 | BMI 22.99 / 24.99 경계 (Normal·Overweight) | off-by-one |
| 13 | BMI 0, 음수 weight — 통계·ID 목록 제외 | 집계 일관성 |
| 14 | age 19, 80 — decade 0, overall 포함 여부 | 연령대 경계 |
| 15 | age 71 — 70대 버킷 | 70대 상한 |
| 16 | 동 대 전원 weight=0 — 보정 실패·건수 0 통계 | 결측 극단 |
| 17 | 체중·키 동시 0 — 순서·최종 BMI | FI-02 파이프라인 |
| 18 | `getNormalBmiUserIds` 복수·0건 | FI-03 |
| 19 | 단일 연령대만 데이터 — 타 대 비율 0 | FI-01 |
| 20 | 헤더 없는 CSV / 컬럼 3개 | 파싱 강건성 |
| 21 | `categoryFromTypeCode` 100/200/300/400 각각 | 레거시 매핑 단위 |
| 22 | `shealth.dat` 골든 샘플 N명·합 100% | 회귀(선택) |

---

## 9) 컴포넌트 책임 (테스트 단위 매핑)

| 컴포넌트 | 단위 테스트 대상 | 통합(`SHealth`) |
|----------|------------------|-----------------|
| `BmiCalculator` | compute, classify, applyToAll | ✓ |
| `DataImputer` | weight/height 보정 | ✓ |
| `CsvReader` | load, skip 규칙 | ✓ (파일 TC) |
| `BmiStatistics` | 비율·ID·분모 0 | ✓ |
| `BmiTypes` | `categoryFromTypeCode`, `AgeDecade::*` | 부분 ✓ |
| `SHealth` | processFile, 파사드 API | ✓ |

---

## 10) 요약: README 대비 구현 성숙도

| 영역 | README | 구현 | 테스트 |
|------|--------|------|--------|
| BMI 공식·4분류 | ✓ | ✓ (25.0 포함) | 경계 일부 |
| 체중 0 보정 | ✓ | ✓ | ✓ |
| 키 0 보정 | 기능 개선 | ✓ | ✓ |
| 연령대 20~70·10년 | 부분 명시 | ✓ (70~79→70대) | 간접 |
| 연령대/전체 비율 | 기능 개선 | ✓ | ✓ |
| 정상 BMI ID 목록 | 기능 개선 | ✓ | ✓ |
| SRP 분리 | 기능 개선 | ✓ | — |
| CSV 오류 처리 | — | skip 위주 | 일부 |

**QA 핵심 리스크**: (1) `bmi≤0`은 분류상 Underweight이나 통계 제외, (2) 연령 20 미만·80 이상은 전체만 반영, (3) 결측은 `==0`만, (4) README 헤더·연령 상한 문구와 구현 세부의 문서화 갭.

---

*문서 생성: README + `src/main/cpp` / `src/test/cpp` 정적 분석 기준.*

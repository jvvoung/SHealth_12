# Code smell analysis for SHealth BMI project
_Exported on 5/19/2026 at 17:27:29 GMT+9 from Cursor (3.4.20)_

---

**User**

# SHealth BMI C++ 프로젝트 — 코드 스멜 분석 프롬프트

## 역할
당신은 C++ 클린코드/리팩토링 전문가입니다. 아래 프로젝트의 코드를 분석하고 코드 스멜을 체계적으로 목록화해 주세요.

## 프로젝트 개요
- **목적**: 삼성 헬스 BMI 통계 — CSV(id, age, weight, height)에서 BMI 계산 후 연령대별 저체중/정상/과체중/비만 비율 산출
- **BMI 공식**: weight(kg) / (height(m))²
- **분류 기준**: ≤18.5 저체중, 18.5초과~23미만 정상, 23이상~25미만 과체중, 25이상 비만
- **보정 규칙**: weight=0이면 같은 연령대(10년 단위) 평균 체중으로 대체
- **구조**:
  - `src/main/cpp/SHealth.h` — 클래스 선언
  - `src/main/cpp/SHealth.cpp` — 핵심 로직
  - `src/main/cpp/SHealthBMI.cpp` — main 진입점
  - `src/test/cpp/SHealthBMITest.cpp` — Google Test

## 분석 요청
1. **코드 구조 이해**: 클래스/함수 책임, 데이터 흐름, 의존 관계를 간단히 설명
2. **코드 스멜 탐지**: 아래 관점에서 구체적 위치(파일명, 함수명, 줄 번호)와 함께 목록화
   - SRP / God Class
   - Long Method / Long Parameter List
   - Magic Number / Hardcoding
   - DRY 위반 (중복 루프, if-else 체인)
   - Primitive Obsession / Data Clumps
   - Poor Naming (함수명과 동작 불일치 포함)
   - 에러 처리 부재 (0으로 나누기, 배열 경계, 파싱 예외)
   - 테스트 가능성 / 테스트 부재
   - 경계값 버그 (BMI 18.5, 23, 25 등)
   - 상태ful 설계, 캡슐화 부족
3. **심각도 분류**: Critical / Major / Minor 로 구분
4. **리팩토링 우선순위**: 1차(네이밍·하드코드·함수 추출·중복 제거) → 2차(SRP 분리·기능 추가) 순으로 제안

## 출력 형식
### 1. 구조 요약 (표 또는 bullet)
### 2. 코드 스멜 목록
| # | 스멜 유형 | 위치 | 설명 | 심각도 |
### 3. 발견된 버그/누락 요구사항
### 4. 1차 리팩토링 체크리스트 (실행 가능한 항목)

## 참고: README 후속 활동
- 1차 리팩토링: 네이밍, 하드코드/전역변수 제거, 함수 추출, 중복 제거
- Unit Test: BMI 계산, 연령대 평균 보정, 분류, 예외
- 기능 개선: SRP 분리, 연령대 BMI 분포, height=0 보정, 정상 BMI 사용자 목록, 전체 비율

---

**Cursor**

프로젝트 소스와 테스트를 읽어 구조와 코드 스멜을 분석합니다.


소스·테스트·README를 기준으로 정리한 코드 스멜 분석입니다.

---

### 1. 구조 요약

| 구성요소 | 책임 | 의존 |
|---------|------|------|
| `SHealthBMI.cpp` | `main` — `shealth.dat` 고정 경로로 `calculateBmi` 호출 후 `printf`로 연령대별 비율 출력 | `SHealth` |
| `SHealth::calculateBmi` | CSV 로드 → 체중 0 보정 → BMI 계산 → 연령대(20~70, 10년 단위)별 4분류 비율 산출·멤버 저장 | `fstream`, `split`, 내부 배열·24개 ratio 멤버 |
| `SHealth::getBmiRatio` | 저장된 비율 조회 (`ageClass` + `type` 100/200/300/400) | `calculateBmi` 선행 실행 가정 |
| `SHealth::split` | CSV 한 줄 파싱 | 없음 |
| `SHealthBMITest.cpp` | 테스트 스켈레톤 (`FAIL()`만 존재) | `SHealth` |

**데이터 흐름**

```
shealth.dat → [ages[], weights[], heights[]] (count건)
           → 연령대별 weight==0 → 동 연령대 평균 체중 대체
           → bmis[] = weight / (height/100)²
           → 연령대별 분류 카운트 → % → underweight20…obesity70
           → getBmiRatio(20|30|…, 100|200|300|400)
```

**설계 특징**

- 단일 클래스 `SHealth`가 I/O, 파싱, 보정, 통계, 조회를 모두 담당 (God Class / SRP 위반).
- 연령·키·몸무게·BMI가 **병렬 고정 배열**(`[10000]`) + **연령대×분류 24개 `double` 멤버**로 표현 (Primitive Obsession, Data Clump).
- 핵심 로직이 private이며 `calculateBmi` 한 번에 묶여 있어 **단위 테스트가 사실상 불가**한 구조.

---

### 2. 코드 스멜 목록

| # | 스멜 유형 | 위치 | 설명 | 심각도 |
|---|----------|------|------|--------|
| 1 | God Class / SRP | `SHealth.h` 6–27행, `SHealth.cpp` 전체 | 파일 I/O, 파싱, imputation, BMI, 집계, 조회 API가 한 클래스에 집중 | Major |
| 2 | Long Method | `SHealth.cpp` `calculateBmi` 6–108행 (~100줄) | 로드·보정·계산·집계·저장이 한 함수에 연속 | Major |
| 3 | Misleading Name | `SHealth.cpp` `calculateBmi` 6행 | BMI만 계산하지 않고 CSV 처리·통계까지 수행 | Minor |
| 4 | Magic Number | `SHealth.cpp` 29, 56행 (`20`~`70`, `+=10`) | 연령대 구간이 하드코딩 | Minor |
| 5 | Magic Number | `SHealth.cpp` 52, 65–71행 | `100.0`, `18.5`, `23`, `25` 상수화 없음 | Minor |
| 6 | Magic Number | `SHealth.h` 13–16행, `getBmiRatio` 111–136행 | 배열 크기 `10000`, type `100/200/300/400` | Major |
| 7 | DRY 위반 | `SHealth.cpp` 76–106행 | `a==20/30/…`마다 동일 4줄 할당 반복 | Major |
| 8 | DRY 위반 | `SHealth.cpp` 111–136행 | `getBmiRatio` 24분기 if-else 체인 | Major |
| 9 | DRY 위반 | `SHealth.cpp` 29–47, 56–75행 | 동일 연령대 필터·전체 순회 루프 3회 | Major |
| 10 | DRY 위반 | `SHealthBMI.cpp` 8–25행 | 연령대별 `printf` 6회 거의 동일 | Minor |
| 11 | Primitive Obsession / Data Clump | `SHealth.h` 13–25행 | `ages/heights/weights/bmis` + 24개 ratio 멤버 | Major |
| 12 | Long Parameter List (의미 부재) | `getBmiRatio(int ageClass, int type)` | `type`이 enum/구조체 없이 정수 코드 | Major |
| 13 | Poor Naming | `getBmiRatio`, `type` 100/200/300/400 | “비율”이지만 분류 종류를 불명확한 정수로 전달 | Major |
| 14 | 에러 처리 부재 | `SHealth.cpp` 21–23행 | `tokens` 개수 검증 없이 `[1]~[3]` 접근 → 짧은 줄 시 UB/예외 | Critical |
| 15 | 에러 처리 부재 | `SHealth.cpp` 21–23행 | `stoi`/`stod` 예외 미처리 | Major |
| 16 | 0으로 나누기 | `SHealth.cpp` 44행 | 연령대 내 유효 체중이 없으면 `sum/ageCount`에서 `ageCount==0` | Critical |
| 17 | 0으로 나누기 | `SHealth.cpp` 52행 | `height==0`이면 BMI 계산 시 분모 0 (height 보정 없음) | Critical |
| 18 | 0으로 나누기 | `SHealth.cpp` 77–80행 등 | 해당 연령대 인원 `sum==0`이면 `* 100 / sum` | Critical |
| 19 | 배열 경계 | `SHealth.cpp` 24행, `SHealth.h` 13행 | `count++` 상한 없음 → 10000건 초과 시 버퍼 오버플로 | Major |
| 20 | 경계값 버그 | `SHealth.cpp` 71행 | 비만 조건 `> 25` → **BMI=25 미분류** (요구: 25**이상**) | Critical |
| 21 | 경계값 불일치 | `SHealth.cpp` 65–71행 | README “23이상~25**미만** 과체중”과 코드 `>=23 && <25`는 일치; “25이상 비만”만 불일치 | Critical |
| 22 | 경계값 누락 | `SHealth.cpp` 65–73행 | `18.5 < bmi < 23` 등으로 대부분 커버되나, **bmi==25**는 어느 분기에도 없음 | Critical |
| 23 | 상태ful / 캡슐화 부족 | `SHealth.h` 12–25행 | 대량 mutable 상태; ratio 멤버는 `calculateBmi` 재호출 전 초기화 없음(빈 연령대는 #18과 결합) | Major |
| 24 | 반환값 모호 | `SHealth.cpp` 9–11, 108행 | 실패 시 `0`, 성공 시 `count` — 오류와 “0건” 구분 불가 | Major |
| 25 | 제어 흐름 버그 | `SHealth.cpp` 18–19행 | `tokens.empty()`이면 **전체 파일 읽기 중단** (빈 줄 하나로 조기 종료) | Major |
| 26 | 테스트 부재 | `SHealthBMITest.cpp` 4–6행 | `FAIL()`만 있어 CI 항상 실패 | Major |
| 27 | 테스트 가능성 | `SHealth` private 로직 | BMI/분류/보정을 독립 검증할 public·테스트 훅 없음 | Major |
| 28 | 요구사항 누락 | 전역 | `height=0` 평균 보정, 전체 비율, 정상 BMI 사용자 목록, BMI 분포 등 README 4단계 기능 없음 | Minor (과제 범위) |
| 29 | 연령대 커버리지 | `SHealth.cpp` 29, 63행 | 20~70만 집계 — 10대·80대 등은 BMI는 계산되나 통계 제외 | Minor |
| 30 | 혼합 스타일 | `SHealthBMI.cpp` vs `SHealth.cpp` | `printf` vs `iostream`/`cerr` 혼용 | Minor |
| 31 | 하드코딩 | `SHealthBMI.cpp` 6행 | 입력 파일 `"shealth.dat"` 고정, 경로/CLI 없음 | Minor |
| 32 | 미사용 데이터 | `SHealth.cpp` 17–23행 | `tokens[0]`(id) 무시 — 문제는 아니나 도메인 모델 부재 | Minor |

---

### 3. 발견된 버그 / 누락 요구사항

**버그 (런타임·정확도)**

1. **BMI = 25.0 미분류** — `obesity` 조건이 `> 25`라 25.0은 어떤 카운터에도 들어가지 않음 (`SHealth.cpp` 71행).
2. **연령대 내 전원 체중 0 또는 보정 불가** — `ageCount == 0`일 때 44행에서 0으로 나눔.
3. **키 0** — 52행 BMI 계산 시 0으로 나눔; README 후속 과제의 height 보정 미구현.
4. **빈 연령대** — `sum == 0`인데 77행 이후 `100 / sum` 수행.
5. **CSV 행 손상** — 컬럼 부족 시 `stoi`/`stod` 또는 벡터 범위 초과.
6. **빈 줄** — 18–19행으로 파일 중간 이후 데이터 무시 가능.
7. **테스트** — `FailedTest`가 `FAIL()`이라 `ctest`는 항상 실패.

**요구사항 대비 갭**

| 요구사항 | 현재 상태 |
|---------|----------|
| weight=0 → 동 연령대 평균 대체 | 구현됨 (단, #2 예외) |
| BMI 분류 경계 (18.5, 23, 25) | 25 경계 오류 (#20) |
| height=0 보정 | 없음 |
| 전체 사용자 BMI 비율 | 없음 |
| 정상 BMI 사용자 목록 | 없음 |
| 연령대 BMI 분포(세분화) | 4분류 비율만, 구조 확장 어려움 |
| 단위 테스트 | 스텁만 존재 |

---

### 4. 1차 리팩토링 체크리스트 (실행 가능)

**네이밍·의미**

- [ ] `calculateBmi` → `loadAndAnalyze` 또는 `processFile` + `computeBmiStatistics` 등으로 역할 분리·이름 정렬
- [ ] `getBmiRatio(ageClass, type)` → `enum class AgeDecade`, `enum class BmiCategory` 또는 `getCategoryRatio(AgeDecade, BmiCategory)`
- [ ] `type` 100/200/300/400 제거

**하드코드·전역 상태**

- [ ] BMI/연령 상수를 `namespace` 또는 `constexpr`로 (`kUnderweightMax`, `kNormalMax`, …)
- [ ] 24개 `underweight20`… 멤버 → `std::array<std::array<double, 4>, 6>` 또는 `map<AgeDecade, CategoryRatios>`
- [ ] 고정 `ages[10000]` → `std::vector<PersonRecord>` (또는 구조체 + vector)

**함수 추출 (SRP 1차)**

- [ ] `loadRecordsFromCsv(path)` — 파싱·검증·예외
- [ ] `imputeMissingWeightsByAgeDecade(records)`
- [ ] `computeBmi(record)` — height cm→m, 0 방어
- [ ] `classifyBmi(double bmi) -> BmiCategory` — 경계 **한 곳**만 정의 (25 포함 `>= kObesityMin`)
- [ ] `computeDecadeStatistics(records) -> DecadeStats`
- [ ] `split`은 유지하거나 `std::stringstream`/`std::getline` 유틸로 정리

**중복 제거**

- [ ] 연령대 루프 `for (decade : kDecades)` 한 번으로 보정·집계·저장 통합
- [ ] 76–106행 `if (a==20)…` → 인덱스/decade 키로 배열에 기록
- [ ] `getBmiRatio` 24분기 → 배열/맵 조회
- [ ] `SHealthBMI.cpp` 출력 루프 1개 + 포맷 문자열

**안전성 (1차에 포함 권장)**

- [ ] `tokens.size() >= 4` 검사, 실패 시 해당 행 스킵·로그
- [ ] `ageCount == 0`, `sum == 0`, `height == 0` 방어 (스킵 또는 명시적 예외/에러 코드)
- [ ] `obesity`: `bmis[i] >= 25` (요구사항 “25이상”)
- [ ] `tokens.empty()` 시 `break` 대신 `continue`
- [ ] `calculateBmi` 반환: `std::optional<size_t>` 또는 `Result` 타입으로 실패/성공 구분

**테스트 (1차 직후)**

- [ ] `FailedTest` 제거 후 `classifyBmi`, `computeBmi`, 보정, 경계값(18.5, 23, **25**) TC 추가
- [ ] fixture CSV 또는 인메모리 문자열로 `load`/`analyze` 분리 테스트

---

### 5. 2차 리팩토링 우선순위 (참고)

1. **CsvReader**, **BmiCalculator**, **WeightImputer**, **AgeDecadeStatistics** 등 SRP 분리  
2. README 4단계: height=0 보정, 전체 비율, 정상 BMI 사용자 ID 목록, 연령대 분포 API  
3. `main`에서 파일 경로 인자화, 출력을 도메인 객체 기반으로 변경  

---

**핵심 결론:** 가장 먼저 고칠 항목은 **BMI=25 미분류**, **0으로 나누기(키·빈 연령대·보정 실패)**, **CSV 검증·빈 줄 처리**, 그다음 **24개 멤버/거대 `calculateBmi` 분해**와 **테스트 스텁 교체**입니다. 1차 리팩토링만으로도 Correctness와 유지보수성이 크게 좋아집니다.

원하시면 다음 단계로 1차 리팩토링을 코드에 직접 적용하거나, 경계값 위주의 Google Test 초안을 작성해 드리겠습니다.

---

**User**

1차 리팩토링 진행해줘.

---

**Cursor**

1차 리팩토링을 진행합니다. 현재 코드를 확인한 뒤 네이밍·상수화·함수 분리·중복 제거·안전성 수정을 적용합니다.


헤더·구현 분리, 상수·enum 도입, 버그 수정, 기본 단위 테스트 추가를 진행합니다.


임시 테스트의 잘못된 assertion을 수정하고, 사용하지 않는 코드를 제거한 뒤 빌드합니다.


1차 리팩토링을 적용했고, 빌드·테스트 5건·`SHealthBMI` 실행까지 확인했습니다.

## 변경 요약

### 구조·네이밍
- `calculateBmi` → **`processFile`** (CSV 로드 + 분석 전체 파이프라인)
- **`HealthRecord`** 구조체 + `std::vector`로 고정 배열 4개 제거
- **`BmiCategory` enum**, **`BmiThresholds`** 상수 namespace 도입
- 24개 ratio 멤버 → **`std::array<double, 4>[6]`** (`categoryRatios_`)

### 함수 분리 (`SHealth.cpp`)
| 함수 | 역할 |
|------|------|
| `loadRecordsFromFile` | CSV 파싱·검증 |
| `imputeMissingWeights` | 연령대별 체중 0 보정 |
| `computeAllBmis` | BMI 계산 |
| `computeDecadeStatistics` | 연령대별 4분류 비율 |
| `computeBmi` / `classifyBmi` | static, 테스트 가능 |

### 버그·안전성 수정
- BMI **25.0 → 비만** (`>= 25`)
- **height ≤ 0** → BMI 0, 통계에서 제외
- **ageCount/sum == 0** → 0으로 나누지 않음
- 잘못된 CSV 행·파싱 예외 → **skip** (`continue`)
- 빈 줄 → **break 대신 continue**
- 파일 오픈 실패 → **`-1` 반환** (성공 시 건수)

### API
- **`getCategoryRatio(ageDecade, BmiCategory)`** 추가
- **`getBmiRatio(ageDecade, typeCode)`** 유지 (100/200/300/400 → 내부 enum 매핑)

### `SHealthBMI.cpp`
- 연령대별 `printf` 6회 → **루프 1회**
- `processFile` 실패 시 exit code 1

### 테스트 (`SHealthBMITest.cpp`)
- `FAIL()` 제거
- BMI 계산, 경계값 분류(18.5/23/25), 체중 보정, 레거시 type 코드, 잘못된 행 skip TC 추가

## 실행 방법

```powershell
cd c:\DEV\SHealth_12\build
ctest --output-on-failure

cd c:\DEV\SHealth_12
.\build\Debug\SHealthBMI.exe   # shealth.dat는 프로젝트 루트에서 실행
```

비만(25.0) 경계 수정으로 20대 비만 비율 등 일부 수치는 이전과 소폭 달라질 수 있습니다. 2차(SRP 클래스 분리, height=0 보정, 전체 비율 등)도 진행할까요?

---

**User**

Report폴더를 만들고 Report폴더에 현재 진행한 내용을 바탕으로 새로운 보고서를 작성해줘.
파일명은 1차_Refactoring_보고서 라고하면될거같아

---

**Cursor**

보고서 작성을 위해 리팩토링 후 코드 상태를 확인합니다.


`Report` 폴더를 만들고 보고서를 작성해 두었습니다.

**경로:** `c:\DEV\SHealth_12\Report\1차_Refactoring_보고서.md`

### 보고서 구성

1. **개요** — 프로젝트 목적, BMI 분류 기준, 대상 파일  
2. **초기 코드 스멜 분석 요약** — God Class, DRY, Magic Number, Critical 버그 7건  
3. **1차 리팩토링 목표·범위** — README 1차 항목 대비 수행/미수행  
4. **상세 변경 내역** — 네이밍, 상수/enum, 데이터 구조, 함수 분리, 중복 제거, 버그 수정  
5. **Before / After** — 규모, 설계 품질, `shealth.dat` 실행 결과  
6. **단위 테스트** — 5개 TC 목록 및 `ctest` 통과 결과  
7. **해결 vs 잔여 이슈** — 2차 과제 정리  
8. **회고** — 달성, 한계, 다음 단계  
9. **부록** — API 변경 예시 (`calculateBmi` → `processFile` 등)

형식은 Markdown(`.md`)입니다. Word/PDF가 필요하면 알려 주세요.

---

**User**

2차 리팩토링 진행해줘.

---

**Cursor**

2차 리팩토링 범위를 파악하기 위해 현재 코드와 README 요구사항을 확인합니다.


SRP 기반 클래스 분리와 README 4단계 기능을 구현합니다.


2차 리팩토링을 완료했습니다. 빌드·테스트 9건·`SHealthBMI` 실행까지 확인했습니다.

## 아키텍처 (SRP 분리)

| 클래스 | 책임 |
|--------|------|
| `BmiTypes` | `HealthRecord`, `BmiCategory`, 연령대 상수 |
| `CsvReader` | CSV 로드·파싱 |
| `BmiCalculator` | BMI 계산·분류 |
| `DataImputer` | 연령대별 **체중·키** 0 보정 |
| `BmiStatistics` | 연령대/전체 비율, 정상 BMI 사용자 ID |
| `SHealth` | 파사드 — `processFile`에서 위 컴포넌트 조율 |

## 신규 기능 (README 4단계)

| API | 설명 |
|-----|------|
| `getDecadeDistribution(ageDecade)` | 연령대 BMI 4분류 비율 배열 |
| `getOverallDistribution()` | **전체 사용자** BMI 비율 |
| `getOverallCategoryRatio(category)` | 전체 단일 분류 비율 |
| `getNormalBmiUserIds()` | **정상 BMI** 사용자 ID 목록 |
| `DataImputer` | `heightCm == 0` → 동 연령대 평균 키 대체 |

## 하위 호환

- `processFile`, `getCategoryRatio`, `getBmiRatio`, `computeBmi`, `classifyBmi` 유지
- 1차 API·type 코드(100~400) 그대로 동작

## 프로젝트 구조

```
src/main/cpp/
  BmiTypes.h / .cpp
  CsvReader.h / .cpp
  BmiCalculator.h / .cpp
  DataImputer.h / .cpp
  BmiStatistics.h / .cpp
  SHealth.h / .cpp      ← 파사드
  SHealthBMI.cpp          ← 전체/정상 사용자 출력 추가
```

## 테스트 (9건 통과)

- 기존 5건 + `ImputeMissingHeightByDecade`, `DecadeDistribution`, `OverallCategoryRatio`, `NormalBmiUserIds`

## 실행 예시 (`shealth.dat`)

- 연령대별 분포 (기존과 동일)
- **전체**: 저체중 1.6%, 정상 13.6%, 과체중 10.4%, 비만 74.5%
- **정상 BMI 사용자**: 654명 (ID 미리보기 10건)

```powershell
cd c:\DEV\SHealth_12\build
cmake --build .
ctest --output-on-failure

cd c:\DEV\SHealth_12
.\build\Debug\SHealthBMI.exe
```

`Report/2차_Refactoring_보고서.md` 작성도 필요하면 말씀해 주세요.

---

**User**

Report/2차_Refactoring_보고서.md 작성 부탁해

---

**Cursor**

2차 리팩토링 보고서 작성을 위해 1차 보고서 형식과 현재 코드 상태를 확인합니다.


`Report/2차_Refactoring_보고서.md` 파일을 작성해 두었습니다.

**경로:** `c:\DEV\SHealth_12\Report\2차_Refactoring_보고서.md`

### 보고서 구성

1. **개요** — 2차 목표(README 4단계) 및 완료 여부  
2. **1차 → 2차 변화** — 구조 Before/After, 파일 트리  
3. **클래스별 SRP** — 6개 클래스 책임·파이프라인·Mermaid 다이어그램  
4. **신규·변경 API** — `getDecadeDistribution`, 전체 비율, 정상 BMI ID 등  
5. **기능 상세** — 키 보정, 전체 통계, ID 목록  
6. **`SHealthBMI` 출력** — 실행 결과 예시  
7. **단위 테스트** — 9개 TC 목록 및 통과 결과  
8. **CMake 변경**  
9. **1차 대비 개선 지표** 표  
10. **잔여·향후 과제**  
11. **회고** — 달성, 설계 선택, 한계  
12. **부록** — `processFile`·신규 API 코드 예시  

1차 보고서와 동일한 형식으로 작성했으며, 상호 링크(`1차_Refactoring_보고서.md`)도 포함했습니다.

---

**User**

코드스멜 탐지 부탁해. 네이밍·매직넘버·함수추출·중복이 남아있는지 확인해줘.

---

**Cursor**

리팩토링 후 코드 전체를 검토해 남아 있는 코드 스멜을 확인합니다.


2차 리팩토링 이후 코드를 **네이밍·매직넘버·함수 추출·중복** 관점에서 재점검한 결과입니다. 초기 God Class 수준의 스멜은 대부분 해소되었고, **잔여 이슈는 주로 Minor~Major**입니다.

---

## 1. 종합 평가

| 관점 | 1차 전 | 2차 후 | 잔여 |
|------|--------|--------|------|
| **네이밍** | Critical~Major 다수 | 대부분 개선 | Minor~Major 일부 |
| **매직넘버** | BMI·연령대·type 코드 산재 | `BmiThresholds`, `AgeDecade` 상수화 | 분산·중복 잔존 |
| **함수 추출** | `calculateBmi` 100줄 | 클래스·파이프라인 분리 | `DataImputer` 등 국소 미추출 |
| **중복** | 24분기·3중 루프 | 배열·공통 함수화 | 보정·집계·파사드 위임 |

**결론:** README 1차 목표(네이밍·하드코드·함수 추출·중복)는 **대체로 달성**되었습니다. 다만 **완전 제거는 아니며**, 아래 항목은 추가 정리 여지가 있습니다.

---

## 2. 코드 스멜 목록

| # | 유형 | 위치 | 설명 | 심각도 |
|---|------|------|------|--------|
| N1 | Poor Naming | `AgeDecade::kCategoryCount` (`BmiTypes.h` 20행) | BMI 분류 개수(4)인데 `AgeDecade` 네임스페이스에 있음 → **의미·소속 불일치** | Minor |
| N2 | Poor Naming | `getBmiRatio` (`SHealth.h` 26행) | “Bmi”인데 반환값은 **비율(%)** → `getCategoryRatio`와 역할 중복·혼동 | Minor |
| N3 | Misleading Default | `categoryFromTypeCode` (`BmiTypes.cpp` 13~14행) | 잘못된 type → `Underweight` 반환. **오류·미지 값** 구분 없음 | Major |
| N4 | 레거시 네이밍 | `typeCode`, `kType100~400` | 1차 호환용으로 남음. 신규 코드는 `BmiCategory` 권장 | Minor |
| N5 | Thin Wrapper | `SHealth.cpp` 47~60행 | `computeBmi`/`classifyBmi`/`ageDecadeStart` 등 **1줄 위임** 4개 (호환 목적이면 허용) | Minor |
| M1 | Magic Number (중복) | `BmiTypes.cpp` 5~11행 vs `SHealth.h` 18~21행 | `100/200/300/400`이 **두 곳**에 정의. `categoryFromTypeCode`만 별도 리터럴 | Major |
| M2 | Magic Number | `BmiCalculator.cpp` 7행 | `100.0` (cm→m). `kCentimetersPerMeter` 등 없음 | Minor |
| M3 | Magic Number | `BmiStatistics.cpp` 13행 | `100.0` (비율 %). `kPercentFactor` 등 없음 | Minor |
| M4 | Magic Number | `CsvReader.cpp` 24, 30~33행 | `tokens.size() < 4`, 인덱스 `0~3` — CSV 컬럼 의미 불명시 | Minor |
| M5 | Magic Number | `SHealthBMI.cpp` 43행 | 미리보기 `10`건 하드코딩 | Minor |
| M6 | Sentinel | `DataImputer`/`BmiStatistics` | `== 0.0`, `bmi <= 0.0` — **결측·무효** 의미가 코드에만 암시 | Minor |
| F1 | 함수 추출 미흡 | `DataImputer.cpp` 4~54행 | `imputeWeights*` / `imputeHeights*` **거의 동일** (~25줄×2) | Major |
| F2 | 함수 추출 여지 | `BmiStatistics::compute` 26~59행 | 연령대 루프(6회 전체 순회) + 전체 1회 순회 — **집계 로직** 분리·단일 패스 가능 | Minor |
| F3 | 출력 로직 | `SHealthBMI.cpp` 20~37행 | `printDecadeStatistics`는 4칸 수동 인덱싱, `printOverall`은 루프 → **통합 출력 함수** 가능 | Minor |
| D1 | DRY 위반 | `DataImputer.cpp` | 체중/키 보정 **복붙 수준 중복** (가장 두드러짐) | Major |
| D2 | DRY 위반 | `BmiStatistics::compute` | 동일 레코드에 `classify` **연령대 루프 + 전체 루프**에서 반복 | Minor |
| D3 | API 중복 | `getDecadeRatio` vs `getDecadeDistribution` | 단일 값 vs 배열 — 의도적이나 **한쪽만으로도 표현 가능** | Minor |
| D4 | 파사드 위임 | `SHealth.cpp` 23~44행 | 통계 조회 **1줄 위임** 5개 (파사드 패턴상 허용, 중복 느낌은 있음) | Minor |
| D5 | enum 병렬 구조 | `SHealthBMI.cpp` 6~17행 | `categoryName` switch ↔ `BmiCategory` — 라벨 테이블 1곳으로 가능 | Minor |

---

## 3. 영역별 상세

### 3.1 네이밍 — **대부분 양호, 잔여 Minor~Major**

**개선된 점**
- `processFile`, `BmiCategory`, `weightKg`/`heightCm`, 역할별 클래스명(`CsvReader`, `DataImputer` 등)

**남은 이슈**
- `AgeDecade::kCategoryCount` → `BmiCategory` 또는 `kBmiCategoryCount`로 이동 권장
- `getBmiRatio`는 레거시 API; 신규는 `getCategoryRatio`만 사용하는 편이 명확
- `categoryFromTypeCode`의 default가 `Underweight`인 것은 **버그 은폐** 가능 (Major)

```3:15:c:\DEV\SHealth_12\src\main\cpp\BmiTypes.cpp
BmiCategory categoryFromTypeCode(int typeCode) {
    switch (typeCode) {
        case 100:
            return BmiCategory::Underweight;
        // ...
        default:
            return BmiCategory::Underweight;  // ← 잘못된 코드도 저체중 처리
    }
}
```

---

### 3.2 매직넘버 — **핵심 BMI·연령대는 상수화, 주변은 잔존**

**상수화 완료**
- `BmiThresholds::kUnderweightMax` 등
- `AgeDecade::kMin/kMax/kStep`

**잔존**

| 값 | 위치 | 권장 |
|----|------|------|
| `100/200/300/400` | `BmiTypes.cpp` + `SHealth.h` | `BmiTypes` 한 곳만, `categoryFromTypeCode`는 `SHealth::kType*` 참조 |
| `100.0` (cm→m) | `BmiCalculator.cpp` | `constexpr double kCmPerMeter = 100.0` |
| `100.0` (%) | `BmiStatistics.cpp` | `constexpr double kRatioPercent = 100.0` |
| `4`, `0~3` | `CsvReader.cpp` | `kExpectedColumnCount`, enum `Column::Id` 등 |
| `10` | `SHealthBMI.cpp` | `kPreviewUserIdCount` |

---

### 3.3 함수 추출 — **클래스 분리는 완료, 메서드 내부는 일부 미완**

**잘 추출됨**
- CSV / 보정 / 계산 / 통계 / 파사드 파이프라인

**추출 권장 (Major 1건)**

`DataImputer`의 체중·키 보정은 동일 패턴입니다.

```4:27:c:\DEV\SHealth_12\src\main\cpp\DataImputer.cpp
void imputeWeightsByAgeDecade(...) {
    // sum / validCount / average / apply — height 버전과 동일 구조
}
```

→ 예: `imputeFieldByAgeDecade(records, getter, setter)` 또는 `std::function<double&(HealthRecord&)>` 템플릿 1개로 통합.

**추출 여지 (Minor)**
- `BmiStatistics::compute`: 연령대별 6회 full scan → 1 pass + decade 버킷
- `SHealthBMI`: `printCategoryDistribution(ratios, label)` 공통화

---

### 3.4 중복 — **1차 수준 중복은 제거, 국소 DRY 잔존**

| 중복 유형 | 상태 |
|----------|------|
| 24개 ratio 멤버, 24분기 `getBmiRatio` | ✅ 제거 |
| 연령대 `a==20/30/…` 6분기 | ✅ 제거 |
| `SHealthBMI` 6회 동일 printf | ✅ 루프화 |
| 체중/키 보정 이중 구현 | ❌ **남음 (Major)** |
| type 코드 이중 정의 | ❌ **남음 (Major)** |
| `BmiStatistics` 이중 순회·이중 classify | ⚠️ **경미** |
| `SHealth` 파사드 1줄 위임 | ⚠️ 의도적 (허용 범위) |

---

## 4. 심각도 요약

| 심각도 | 건수 | 대표 항목 |
|--------|------|----------|
| **Major** | 3 | `DataImputer` 중복(F1/D1), type 코드 이중 정의(M1), `categoryFromTypeCode` default(N3) |
| **Minor** | 12+ | cm/% 100.0, CSV 인덱스, 출력 통합, 네임스페이스 `kCategoryCount` 등 |

**Critical** 수준 스멜(0 나누기, BMI 25 미분류, God Class)은 **1~2차에서 이미 처리**된 상태로, 이번 점검에서는 재발하지 않았습니다.

---

## 5. 권장 후속 작업 (우선순위)

| 순위 | 작업 | 기대 효과 |
|------|------|----------|
| 1 | `DataImputer` 보정 로직 **단일 함수로 통합** | D1/F1 해소, 유지보수성 ↑ |
| 2 | type 코드 `100~400`을 **`BmiTypes` 단일 정의** + `categoryFromTypeCode` 연동 | M1 해소 |
| 3 | `categoryFromTypeCode` default → `std::optional` 또는 **예외/0 반환** | N3 해소 |
| 4 | `kCmPerMeter`, `kPercentScale`, CSV 컬럼 enum | M2~M4 정리 |
| 5 | `AgeDecade::kCategoryCount` → `kBmiCategoryCount` 등 **이름·위치 정리** | N1 해소 |

---

## 6. 한 줄 결론

**네이밍·매직넘버·함수 추출·중복**은 1차 대비 **크게 개선**되었고, 실무에서 바로 손댈 만한 잔여 스멜은 **`DataImputer` 이중 구현**, **type 코드 이중 정의**, **`categoryFromTypeCode` 오류 처리** 세 가지가 핵심입니다. 나머지는 Minor 수준의 다듬기에 가깝습니다.

원하시면 위 1~3순위만 골라 **3차 미니 리팩토링**으로 바로 적용해 드리겠습니다.

---

**User**

3차 리팩토링도 진행해줘. 

---

**Cursor**

3차 리팩토링(잔여 스멜 제거)을 진행합니다. 관련 파일을 확인한 뒤 수정합니다.


3차 리팩토링 적용 중: 상수 통합, `DataImputer` 일반화, 집계 단일 패스, 출력 통합.


3차 리팩토링을 반영했습니다. 테스트 **10건 모두 통과**했습니다.

## 3차 변경 요약

### 1. `DataImputer` 중복 제거 (Major)
- `imputeWeightsByAgeDecade` / `imputeHeightsByAgeDecade` → **`imputeFieldByAgeDecade(records, &HealthRecord::weightKg|heightCm)`** 단일 구현

### 2. 매직넘버·상수 통합
| 네임스페이스 | 상수 |
|-------------|------|
| `LegacyTypeCode` | `kUnderweight(100)` … `kObesity(400)` — **단일 정의** |
| `BmiCategoryMeta` | `kCount`, `allCategories()`, `categoryName()` |
| `PhysicalUnits` | `kCentimetersPerMeter`, `kMissingValue`, `kInvalidBmi` |
| `StatisticsScale` | `kPercent` (100.0) |
| `CsvReader::Column` | `Id/Age/Weight/Height`, `kColumnCount` |

- `SHealth::kType*` → `LegacyTypeCode::*` **alias**
- `AgeDecade::kCategoryCount` 제거 → `BmiCategoryMeta::kCount`

### 3. `categoryFromTypeCode` 오류 처리 (Major)
- 반환: `std::optional<BmiCategory>`
- 잘못된 code → `std::nullopt`
- `getBmiRatio` → invalid 시 **0.0** 반환 (TC `InvalidTypeCodeReturnsZero` 추가)

### 4. `BmiStatistics` 단일 패스
- 연령대 6회 + 전체 1회 순회 → **레코드 1회 순회**로 decade/overall 동시 집계

### 5. `SHealthBMI` 출력 통합
- `printCategoryDistribution()` 공통 함수
- 분류 라벨 → `BmiCategoryMeta::categoryName()` (중복 switch 제거)
- `kPreviewUserIdCount = 10`

## 테스트
- 기존 9건 + **`InvalidTypeCodeReturnsZero`** → 총 **10건 통과**

## 하위 호환
- `SHealth::kType100~400`, `getBmiRatio`, `processFile` 등 **기존 API 유지**

`Report/3차_Refactoring_보고서.md` 작성이 필요하면 말씀해 주세요.

---

**User**

3차 보고서도 작성해줘 Report폴더에

---

**Cursor**

1·2차 보고서 형식을 참고해 3차 보고서를 작성합니다.


`Report/3차_Refactoring_보고서.md` 파일을 작성해 두었습니다.

**경로:** `c:\DEV\SHealth_12\Report\3차_Refactoring_보고서.md`

### 보고서 구성

1. **개요** — 2차 이후 잔여 스멜 목록 및 3차 목표  
2. **목표·달성** — 8개 항목 체크리스트  
3. **상세 변경** — `DataImputer` 통합, 상수 namespace, `optional`, CSV enum, 단일 패스, 출력 통합  
4. **해결된 스멜 체크리스트** — D1/F1, M1, N3 등  
5. **Before/After** — 규모·품질 지표  
6. **단위 테스트** — 10건 목록 및 결과  
7. **1~3차 누적 요약** — 아키텍처 다이어그램  
8. **잔여 과제** — CLI, 통합 TC 등  
9. **회고** — 달성·원칙·한계  
10. **부록** — 주요 API 예시  

1·2차 보고서와 동일한 형식이며, 상호 링크도 포함했습니다.


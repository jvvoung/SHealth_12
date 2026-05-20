# SHealth BMI (C++)

삼성 헬스 스타일 BMI 통계 계산 실습 프로젝트입니다.  
원본 레거시 코드를 **1~3차 리팩토링**으로 모듈 분리·클린코드·단위 테스트를 적용했으며, `tc` 브랜치에서는 테스트 케이스(TC) 보강 작업을 진행합니다.

## Overview

- 수집 데이터(ID, 나이, 체중 kg, 키 cm)로 **BMI**를 계산하고, 연령대(20대·30대 …)별 **저체중/정상/과체중/비만** 비율을 집계합니다.
- 체중·키가 **0**이면 같은 연령대 평균으로 보정합니다.
- BMI = 체중(kg) / 키(m)²
- 분류 기준: 18.5 이하 저체중, 18.5 초과~23 미만 정상, 23 이상~25 미만 과체중, 25 이상 비만

![BMI](./bmi.png)

## 데이터 샘플

입력 파일: `shealth.dat`

```
id,age,weight,height
93705,66,79.5,158.3
93708,66,53.5,150.2
93709,75,88.8,151.1
...
```

각 행: ID, 나이, 체중(kg), 키(cm)

## 빌드 및 실행

### 요구사항

- CMake 3.10 이상
- C++17 지원 컴파일러 (MSVC, GCC, Clang)
- Google Test (CMake `FetchContent`으로 자동 다운로드)

### 빌드

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Windows (Visual Studio):

```powershell
mkdir build; cd build
cmake ..
cmake --build . --config Debug
```

### 실행

```bash
./SHealthBMI          # Linux / macOS
build\Debug\SHealthBMI.exe   # Windows (Debug)
```

기본 입력 파일: 프로젝트 루트의 `shealth.dat`

### 테스트 실행

```bash
cd build
ctest
# Windows Debug: ctest -C Debug
```

## 프로젝트 구조

```
CMakeLists.txt
shealth.dat
bmi.png
src/
  main/cpp/
    SHealthBMI.cpp      # main (CLI 진입점)
    SHealth.h / .cpp    # 파사드 — 파일 처리·API 위임
    BmiTypes.h / .cpp   # 도메인 타입·상수·분류 메타
    CsvReader.h / .cpp  # CSV 로드 (Column enum)
    DataImputer.h / .cpp # 결측(0) 체중·키 연령대 평균 보정
    BmiCalculator.h / .cpp # BMI 계산·분류
    BmiStatistics.h / .cpp # 연령대·전체 분포·비율 집계
  test/cpp/
    SHealthBMITest.cpp  # Google Test 단위 테스트
Report/                 # 분석·리팩토링 보고서
Prompting/              # Cursor AI용 프롬프트 Export
docs/
  requirements_analysis.md
```

### 아키텍처 (3차 리팩토링 완료)

```
SHealthBMI (main)
    └── SHealth (파사드)
            ├── CsvReader
            ├── DataImputer
            ├── BmiCalculator
            ├── BmiStatistics
            └── BmiTypes
```

## 단위 테스트 (현재 TC)

| 테스트 | 내용 |
|--------|------|
| `ComputeBmi` | BMI 계산 |
| `ClassifyBmiBoundaries` | 경계값 분류 |
| `ImputeMissingWeightByDecade` | 체중 0 → 연령대 평균 보정 |
| `ImputeMissingHeightByDecade` | 키 0 → 연령대 평균 보정 |
| `DecadeDistributionMatchesCategoryRatio` | 연령대 분포·비율 일치 |
| `OverallCategoryRatio` | 전체 비율 |
| `NormalBmiUserIds` | 정상 BMI 사용자 ID 목록 |
| `LegacyTypeCodeCompatibility` | 레거시 type 코드(100~400) |
| `InvalidTypeCodeReturnsZero` | 잘못된 type → 0.0 |
| `SkipsInvalidRows` | 잘못된 CSV 행 스킵 |

## 문서

| 경로 | 설명 |
|------|------|
| [Report/02.요구사항_분석_보고서.md](Report/02.요구사항_분석_보고서.md) | 요구사항 분석 |
| [Report/03.1차_Refactoring_보고서.md](Report/03.1차_Refactoring_보고서.md) | 1차 리팩토링 (클린코드·기본 TC) |
| [Report/03.2차_Refactoring_보고서.md](Report/03.2차_Refactoring_보고서.md) | 2차 리팩토링 (SRP·기능 추가) |
| [Report/03.3차_Refactoring_보고서.md](Report/03.3차_Refactoring_보고서.md) | 3차 리팩토링 (잔여 코드 스멜 제거) |
| [docs/requirements_analysis.md](docs/requirements_analysis.md) | 요구사항 분석 (원문) |
| [Prompting/](Prompting/) | Cursor 대화형 프롬프트 Export |

## 브랜치

| 브랜치 | 용도 |
|--------|------|
| `Refactoring` | 1~3차 리팩토링·문서 완료 기준선 |
| `tc` | 단위 테스트(TC) 보강 및 회귀 검증 |

## 생성형 AI 실습 Activities (6시간)

1. **문제 코드 분석·코드 스멜** (1h) — 구조·BMI 로직 이해, 스멜 식별
2. **1차 리팩토링** (1h) — 네이밍, 하드코드/전역 제거, 함수 추출, 중복 제거
3. **Unit Test 작성** (1h) — BMI·보정·분류·예외 TC
4. **기능 개선** (2h) — SRP 분리, 연령대 분포, height 보정, 정상 ID 목록, 전체 비율
5. **회고·발표** (1h) — Before/After, AI 활용, TC 팁

## 주의 사항

- 코드 품질 향상을 위해 필요 시 **STL** 사용 가능합니다.
- 레거시 API(`getBmiRatio`, static `computeBmi` 등)는 호환을 위해 유지되어 있습니다. 신규 코드는 `getCategoryRatio`, `BmiCategory` enum 사용을 권장합니다.

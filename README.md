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
ctest --output-on-failure
# Windows Debug: ctest -C Debug --output-on-failure
```

### Golden Master 회귀 (TC-25)

`shealth.dat` 기준 `SHealthBMI` 콘솔 출력을 `test/golden/shealth_bmi_report.golden.txt`와 비교합니다.

```bash
ctest -R GoldenMaster --output-on-failure
# 의도적 baseline 갱신 (로컬만)
cmake --build build --target update-golden
```

상세: [docs/golden_master.md](docs/golden_master.md)

## 프로젝트 구조

```
CMakeLists.txt
shealth.dat
bmi.png
src/main/cpp/
  SHealthBMI.cpp      # main (CLI 진입점)
  SHealth.h / .cpp    # 파사드 — 파일 처리·API 위임
  BmiTypes.h / .cpp   # 도메인 타입·상수·분류 메타
  CsvReader.h / .cpp  # CSV 로드 (Column enum)
  DataImputer.h / .cpp # 결측(0) 체중·키 연령대 평균 보정
  BmiCalculator.h / .cpp # BMI 계산·분류
  BmiStatistics.h / .cpp # 연령대·전체 분포·비율 집계
test/
  cpp/
    SHealthBMITest.cpp   # Google Test 단위 테스트
    GoldenMasterTest.cpp # TC-25 골든 회귀 (SHealthBMI stdout)
  golden/
    shealth_bmi_report.golden.txt
    actual/              # 실패 시 diff용 actual 저장
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

# 생성형AI를 활용한 Activities (6 시간)

1. 문제 코드 분석 및 코드 스멜 찾기 (1시간)
- 기본 코드구조, BMI 로직 이해
- 코드 스멜 찾기
2. 1차 리펙토링 (클린코드 관점, 아래 내용을 순차적으로 수행) (1시간)
- 네이밍 개선
- 하드코드 및 전역변수 제거
- 함수 추출
- 반복/중복 제거
3. UnitTest 작성 (1시간)
- BMI 계산 로직 TC
- Age 평균치 보정 로직 TC
- 정상/저체중/과체중/비만 분류 TC
- 예외상황 TC
4. 기능 개선 (2시간)
- SRP에 따른 책임 분리등 리팩토링
- 특정 연령대의 BMI 분포 비율 계산 기능 추가
- Height가 0인 경우에 대한 평균치 보정 로직 추가
- BMI 정상 범위 사용자 목록 조회 기능 추가
- 전체 사용자 대비 각 BMI 범주 비율 계산 기능 추가
5. 회고 및 발표 (1시간)
- 실습 목표와 달성도
- 코드 품질 Before & After
- AI를 어떻게 활용했나? 도움이 된 순간과 한계는?
- TC를 추가해보면서 개선에 미친 영향, TC 작성 팁
- 클린코드와 리팩토링에서 느낀 장점과 어려운점

## 주의 사항

- 코드 품질 향상을 위해 필요 시 **STL** 사용 가능합니다.
- 레거시 API(`getBmiRatio`, static `computeBmi` 등)는 호환을 위해 유지되어 있습니다. 신규 코드는 `getCategoryRatio`, `BmiCategory` enum 사용을 권장합니다.

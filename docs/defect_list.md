# SHealth BMI — 결함 목록 (Defect List)

| 항목 | 내용 |
|------|------|
| **작성일** | 2026-05-20 |
| **작성** | QA 리드 |
| **기준** | `tc` 브랜치 · ctest 36건 · [Report/06.결함_분석_보고서.md](Report/06.결함_분석_보고서.md) |
| **현재 상태** | **활성 테스트 실패 0건** (36/36 Green) · **프로덕션 활성 결함 0건** |

---

## 요약

| 구분 | 건수 |
|------|------|
| 이력 결함 (수정 완료) | 1 |
| 잔존 리스크 / 테스트 갭 | 3 |
| Critical / Major (활성) | 0 |

---

## 결함 상세

### D-01

| 필드 | 내용 |
|------|------|
| **ID** | D-01 |
| **Severity** | Info (이력) — 테스트 오류, 프로덕션 정상 |
| **ItemType** | Test Defect |
| **Steps** | 1. `RecordsPipelineFixture`로 `records_ = {{1, 25, 50.0, 160.0, 0.0}}` 설정<br>2. `runPipeline()` 실행 (DataImputer → BmiCalculator → BmiStatistics)<br>3. `EXPECT_NEAR(statistics_.getOverallRatio(BmiCategory::Underweight), 100.0, 0.01)` 검증<br>4. `ctest -C Debug --output-on-failure` 또는 `BmiStatisticsTest.SingleRecord_OverallHundredPercent` 단독 실행 |
| **Expected** | 단일 유효 레코드 기준 **Underweight** 비율 **100.0%**, 4분류 overall 분포 합 **≈ 100.0%** (TC-22, `docs/test_plan.md`) |
| **Actual** | BMI ≈ **19.531** (Normal) → `getOverallRatio(Underweight)` **0.0%**, `getOverallRatio(Normal)` **100.0%** · 2차 assertion(분포 합 100%)은 통과 가능 |
| **Root Cause** | 픽스처 `50kg/160cm`가 BMI **19.531**로 **Normal**(`18.5 < BMI < 23`)인데, assert는 **Underweight 100%**를 기대함. `BmiCalculator::classify`·`BmiStatistics::compute`는 명세·README와 일치하며 결함 없음 |
| **Fix Summary** | `SHealthBMITest.cpp` 픽스처를 **`45kg/170cm`** (BMI ≈ 15.571, Underweight)로 변경 및 Given 주석 정정. **수정 완료** · 회귀 36/36 Green (2026-05-20 재검증) |

**관련:** TC-22 · `BmiStatisticsTest.SingleRecord_OverallHundredPercent` · Report/05 §9.1, Report/06 §4.2

---

### R-01

| 필드 | 내용 |
|------|------|
| **ID** | R-01 |
| **Severity** | Info |
| **ItemType** | Design Risk (결함 아님, 문서화·인지 필요) |
| **Steps** | 1. `height≤0` 또는 `compute` 결과 `bmi=0`인 레코드 준비<br>2. `BmiCalculator::classify(0)` 호출<br>3. `BmiStatistics::compute` 후 집계·`getNormalBmiUserIds` 조회 |
| **Expected** | (혼동 가능) 분류 API와 통계 API 동작이 동일하다고 가정 |
| **Actual** | `classify(0)` → **Underweight** 반환 · `bmi≤0` 레코드는 **통계·ID 목록에서 제외** (`BmiStatisticsTest.ExcludesNonPositiveBmi`로 검증됨) |
| **Root Cause** | `requirements_analysis.md` §2.2 설계: 분류 함수와 집계 필터 책임 분리. 버그가 아닌 **의도된 불일치** |
| **Fix Summary** | 프로덕션 수정 **불필요**. TC-14·문서로 동작 고정 유지. 신규 API 추가 시 호출부 계약 명시 권장 |

---

### R-02

| 필드 | 내용 |
|------|------|
| **ID** | R-02 |
| **Severity** | Minor |
| **ItemType** | Test Infrastructure Risk |
| **Steps** | 1. 기존 10건 `TEST(SHealthBmiTest, …)` 병렬 실행 환경 구성<br>2. 각 TC가 동일 경로 `test_shealth_temp.csv`에 동시 read/write<br>3. `ctest -j` 또는 CI 병렬 job으로 전체 스위트 실행 |
| **Expected** | 테스트 간 파일 I/O 격리, 재현 가능한 Green |
| **Actual** | 현재 순차 ctest(36/36)에서는 **실패 재현 없음**. 이론적으로 **파일 경합·오염** 가능 |
| **Root Cause** | 레거시 10 TC가 `writeTempCsv` **고정 파일명** 사용. 신규 26 TC는 `CsvProcessFixture`의 `test_shealth_{counter}.csv` + `TearDown` 삭제로 완화됨 |
| **Fix Summary** | P3: 기존 10 TC도 고유 suffix·`TearDown` 패턴으로 통일 (Report/06 §9). **미조치** — 순차 ctest 환경에서는 허용 |

---

### R-03

| 필드 | 내용 |
|------|------|
| **ID** | R-03 |
| **Severity** | Info |
| **ItemType** | Test Gap (미구현 시나리오) |
| **Steps** | 1. `docs/test_plan.md` P2 항목 대조 (TC-24~27, 골든 회귀 등)<br>2. `SHealthBMITest.cpp`·`ctest` 실행<br>3. G3(line coverage ≥88%), `shealth.dat` E2E, CI 미구성 여부 확인 |
| **Expected** | P2: `shealth.dat` 골든 회귀, `SHEALTH_ENABLE_COVERAGE`, `categoryName`/`indexForDecade(10)` 등 · G3 커버리지 게이트 |
| **Actual** | **36 TC Green**이나 P2·G3·CLI main·일부 데이터 계약(음수 weight 등) **미검증** |
| **Root Cause** | 7~8차 범위가 P0/P1(TC-12~22) 구현·결함 분석에 한정. `test_plan.md` 후속 항목 미착수 |
| **Fix Summary** | 9차(P2): 골든 TC-25, CMake coverage 옵션, TC-26/27 추가. P3: GitHub Actions `ctest`. **미조치** (후속 스프린트) |

---

## 심각도·유형 매트릭스

| ID | Severity | ItemType | ctest 실패 | 조치 상태 |
|----|----------|----------|------------|-----------|
| D-01 | Info | Test Defect | 이력 1건 (TC-22) | **Closed** (픽스처 수정) |
| R-01 | Info | Design Risk | 없음 | Open (인지·문서) |
| R-02 | Minor | Test Infrastructure | 없음 | Open (P3 완화) |
| R-03 | Info | Test Gap | 없음 | Open (P2/P3) |

| Severity | 활성 건수 |
|----------|-----------|
| Critical | 0 |
| Major | 0 |
| Minor | 1 (R-02) |
| Info | 3 (D-01 이력 포함) |

---

## 검증 이력

| 일시 | 명령 | 결과 |
|------|------|------|
| 2026-05-20 | `cd build; ctest -C Debug --output-on-failure` | **36 passed, 0 failed** (~0.82s) |
| 2026-05-20 | `BmiStatisticsTest.SingleRecord_OverallHundredPercent` | **Passed** (D-01 수정 반영) |

---

## 참조

| 문서 | 역할 |
|------|------|
| [Report/06.결함_분석_보고서.md](Report/06.결함_분석_보고서.md) | 8차 QA 결함 분석 원본 |
| [Report/05.TC_구현_보고서.md](Report/05.TC_구현_보고서.md) | TC-22 1차 실패·수정 배경 |
| [docs/test_plan.md](docs/test_plan.md) | TC ID·P2 설계 SoT |
| [src/test/cpp/SHealthBMITest.cpp](src/test/cpp/SHealthBMITest.cpp) | 36 TC 실행본 |

---

*본 목록은 현재까지 발견·분류된 테스트 실패 및 결함·리스크를 QA 형식으로 정리한 것입니다. ctest 실패 발생 시 본 문서와 `Report/06`을 동기화하세요.*

# Golden Master 회귀 테스트 (TC-25)

`SHealthBMI` 콘솔 리포트를 `shealth.dat` 기준으로 고정하고, 의도치 않은 출력·통계 변경을 자동 감지합니다.

## 구조

| 경로 | 설명 |
|------|------|
| `test/golden/shealth_bmi_report.golden.txt` | 기대 stdout (정규화 후) |
| `test/golden/actual/` | 실패 시 실제 출력 저장 (`*.actual.txt`) |
| `test/cpp/GoldenMasterTest.cpp` | Google Test — `SHealthBMI` subprocess + 파일 비교 |
| `shealth.dat` | 프로젝트 루트 실데이터 (기준선) |

## 설계 (Option A)

- **E2E**: 테스트가 `SHealthBMI` 실행 파일을 subprocess로 실행해 stdout 캡처.
- **포맷 안정화**: `SHealthBMI.cpp`에서 비율 출력을 `%.2f`로 고정 (MSVC/GCC 간 `%f` 차이 방지).
- **비교 전 정규화**: CRLF→LF, 끝 공백 제거 (`normalizeOutput`).
- **Normal BMI ID**: count + 상위 10건만 golden에 포함 (프로덕션 동작 그대로).

## 실행

```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

Windows (Visual Studio 다중 구성):

```powershell
cmake --build build --config Debug
cd build
ctest -C Debug --output-on-failure
```

Golden 테스트만:

```bash
ctest -R GoldenMaster --output-on-failure
```

## Golden 갱신 (의도적 변경 시만)

로컬에서만 실행하세요. **CI에서는 `UPDATE_GOLDEN`을 설정하지 않습니다.**

```bash
cmake --build build --target update-golden
# Windows: cmake --build build --config Debug --target update-golden
```

또는:

```bash
# Linux / macOS
UPDATE_GOLDEN=1 ctest -R GoldenMaster --output-on-failure

# PowerShell
$env:UPDATE_GOLDEN=1; ctest -C Debug -R GoldenMaster --output-on-failure
```

갱신 후 `test/golden/shealth_bmi_report.golden.txt` diff를 검토하고 커밋합니다.

### `shealth.dat` 변경 시

1. `shealth.dat` 수정·교체
2. `update-golden` 실행
3. golden diff 리뷰 (연령대 비율·overall %·Normal count·id 미리보기 확인)
4. golden + `shealth.dat` 함께 커밋

### BMI 로직·출력 포맷 변경 시

1. 코드 변경 후 전체 `ctest` Green 확인
2. 의도된 출력이면 `update-golden`으로 baseline 재생성
3. PR에 golden diff와 변경 사유 명시

## CMake 옵션

| 옵션 | 기본 | 설명 |
|------|------|------|
| `SHEALTH_ENABLE_GOLDEN_TESTS` | `ON` | `OFF` 시 Golden TC는 `GTEST_SKIP` |
| `update-golden` 타깃 | — | `UPDATE_GOLDEN=1`로 golden 파일 재생성 |

## 실패 시

1. `test/golden/actual/shealth_bmi_report.actual.txt` 와 golden diff
2. `shealth.dat` / 통계 로직 / 출력 포맷 중 무엇이 바뀌었는지 확인
3. 버그면 코드 수정, 의도된 변경이면 golden 갱신

CI 실패 시 GitHub Actions 아티팩트 `golden-actual-<os>`에서 actual 파일을 받을 수 있습니다.

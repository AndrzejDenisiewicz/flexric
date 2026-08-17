<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# FlexRIC Documentation

[![License](https://img.shields.io/badge/License-CSSL%20v1.0-blue)](LICENSE)
[![Ubuntu 22.04](https://img.shields.io/badge/Ubuntu-22.04-E95420?logo=ubuntu\&logoColor=white)](https://releases.ubuntu.com/22.04/)
[![RHEL 9](https://img.shields.io/badge/RHEL-9-EE0000?logo=redhat\&logoColor=white)](https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/9)
[![Docker Hub](https://img.shields.io/badge/Docker%20Hub-oai--flexric-2496ED?logo=docker\&logoColor=white)](https://hub.docker.com/r/oaisoftwarealliance/oai-flexric/tags)

This repository contains [O-RAN Alliance](https://www.o-ran.org/) compliant E2 Node Agent emulators, a nearRT-RIC, and xApps written in C/C++ and Python.

It implements various service models, including O-RAN standard **E2SM-KPM v2.01/v2.03/v3.00** and **E2SM-RC v1.03**, as well as customized **NG/GTP, PDCP, RLC, MAC, SC, and TC** service models.

Depending on the service model, different encoding schemes are supported, including **ASN.1, FlatBuffers, and plain encoding**.

The indication data received by the xApps can be persisted in an **SQLite3** database, enabling offline processing applications such as ML/AI.

FlexRIC also supports **E2AP v1.01/v2.03/v3.01** for all supported service models.

## License

* [OAI License Model](http://www.openairinterface.org/?page_id=101)
* [CSSL v1.0](http://www.openairinterface.org/?page_id=698)

The source code is distributed under [**CSSL v1.0**](LICENSE).

Files under `src/util/alg_ds`, Docker Compose YAML files, and selected CI scripts are distributed under the [**MIT License**](LICENSES/preferred/MIT.txt).

Documentation is distributed under the [**Creative Commons Attribution 4.0 International License**](LICENSES/preferred/CC-BY-4.0.txt).

Please see [NOTICE](NOTICE) for information about third-party software used by FlexRIC.

## Documentation

The FlexRIC documentation and tutorials are available in the [documentation repository](https://github.com/duranta-project/docs/tree/main/flexric).

## Contributing

Please see [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

## Repository Structure

```text
.
├── ci-scripts/       # CI and testing scripts
├── conf_files/       # Configuration files
├── docker/           # Dockerfiles and Docker Compose configuration
├── examples/         # Example E2 agents, RIC, and xApps
├── grafana/          # Grafana dashboards and data sources
├── openshift/        # OpenShift deployment configuration
├── src/              # Source files
├── test/             # Test files and test configurations
├── CMakeLists.txt    # Main CMake build configuration
├── CONTRIBUTING.md   # Contribution guidelines
├── LICENSE           # Project license
├── LICENSES/         # Additional license files
├── NOTICE            # Third-party software notices
```

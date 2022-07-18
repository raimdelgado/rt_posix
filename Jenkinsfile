pipeline {
	agent any
	stages {
		stage('BUILD') {
			when{
				expression{
					currentBuild.result == null || currentBuild.result == 'SUCCESS'
				}
			  }
			steps {
				sh 'make all'
			}
		}
		stage('STATIC ANALYSIS'){
			steps{
				sh '''cd ${WORKSPACE}
					cppcheck ./ --enable=all --suppress=missingIncludeSystem -I include/ --xml --xml-version=2 --platform=unix64 2> CppCheck.xml'''
				
				recordIssues(enabledForFailure: true, aggregatingResults: true, tools: [cppCheck(pattern: 'CppCheck.xml')])
			}
		}
		stage ('UNIT TEST AND CODE COVERAGE'){
			steps{
				sh '''cd ${WORKSPACE}
				 	cd ./test
					./start.sh UnitTest --gtest_output=xml:../GTest.xml
					cd ${WORKSPACE}
					lcov -c --directory . --output-file Coverage.info
					lcov -r Coverage.info -o Coverage.info '/usr/include/*'
					genhtml Coverage.info --output-directory html_out
					lcov_cobertura Coverage.info -o Coverage.xml
					'''
				cobertura(coberturaReportFile: 'Coverage.xml', onlyStable: false)
			}
			post {
				success {
					junit(testResults: 'GTest.xml', skipPublishingChecks: true)
				}
			}
		}
	}
}
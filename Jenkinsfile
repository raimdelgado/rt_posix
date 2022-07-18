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
	}
}

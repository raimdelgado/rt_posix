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
  }
}

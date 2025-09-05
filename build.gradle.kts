plugins {
    kotlin("jvm") version "2.1.20"
}

group = "com.daonvshu.protocol.codec"
version = "1.0.3"

repositories {
    mavenCentral()
}

dependencies {
    implementation("org.jetbrains.kotlin:kotlin-reflect")
    compileOnly("org.jetbrains.kotlinx:kotlinx-serialization-json:1.8.1")

    testImplementation(kotlin("test"))
}

tasks.test {
    useJUnitPlatform()
}
kotlin {
    jvmToolchain(17)
}

sourceSets {
    main {
        kotlin {
            exclude("com.daonvshu.protocol.codec.Main.kt")
        }
    }
}

tasks.register<Jar>("fatJar") {
    group = "build"
    description = "生成包含所有依赖的可执行JAR包"

    duplicatesStrategy = DuplicatesStrategy.EXCLUDE

    from(sourceSets.main.get().output)

    from({
        configurations.runtimeClasspath.get().map { if (it.isDirectory) it else zipTree(it) }
    })

    with(tasks.jar.get() as CopySpec)
}

tasks.register<Jar>("sourceJar") {
    archiveClassifier.set("sources")
    from(sourceSets.main.get().allSource)
}
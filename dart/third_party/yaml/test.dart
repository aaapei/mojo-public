import 'package:yaml/yaml.dart';

main() {
  const src = """
name: linter
version: 0.0.1
author: Dart Team <misc@dartlang.org>
authors:
  - Bill
  - Ted
description: Style linter for Dart.
documentation:
homepage: https://github.com/dart-lang/linter
dependencies:
  transmogrify:
    hosted:
      name: transmogrify
      url: http://your-package-server.com
    version: '>=0.4.0 <1.0.0'
  analyzer: '0.24.0-dev.1'
  cli_util: '>=0.0.1 <0.1.0'
  semver: '>=0.2.0 <0.3.0'
  yaml: '>=2.1.2 <3.0.0'
  kittens:
    git:
      url: git://github.com/munificent/kittens.git
      ref: some-branch
  foo: any
dev_dependencies:
  markdown: '>=0.7.1+2 <0.8.0'
  unittest: '>=0.11.0 <0.12.0'
""";

  YamlMap node = loadYamlNode(src, sourceUrl: null);
  node.nodes.forEach((k, v) {
    if (k is YamlScalar) print(k.span);
  });
}

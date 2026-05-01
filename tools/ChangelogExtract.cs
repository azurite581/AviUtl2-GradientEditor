#:package System.CommandLine@2.0.3

// Extract a specific version's section from the Changelog.
// Usage:
//   $ dotnet ChangelogExtract.cs -- <src> <dst> --semver <version>
// Usage example:
//   $ dotnet ChangelogExtract.cs -- ./CHANGELOG.md ./EXTRACTED.md --semver 0.0.5

using System.Text;
using System.CommandLine;

var srcFilePathArgument = new Argument<FileInfo>("src")
{
    Description = "Srource file path"
};

var dstFilePathArgument = new Argument<FileInfo>("dst")
{
    Description = "Destination file path"
};

Option<string> semVerOption = new("--semver")
{
    Description = "Semantic Version",
    Required = true
};

var extractCommand = new Command("extract", "Extract a specific version's section from the Changelog");
extractCommand.Arguments.Add(srcFilePathArgument);
extractCommand.Arguments.Add(dstFilePathArgument);
extractCommand.Options.Add(semVerOption);

extractCommand.SetAction(parseResult =>
{
    var srcFilePath = parseResult.GetValue(srcFilePathArgument);
    var dstFilePath = parseResult.GetValue(dstFilePathArgument);
    string? semVer = parseResult.GetValue(semVerOption);

    if (srcFilePath?.Exists != true)
    {
        Console.Error.WriteLine("File not found.");
        return;
    }

    if (string.IsNullOrWhiteSpace(semVer))
    {
        Console.Error.WriteLine("--semver is required.");
        return;
    }

    string header = $"## [{semVer}]";

    var lines = File.ReadAllLines(srcFilePath.FullName, Encoding.UTF8);

    int start = Array.FindIndex(lines, l => l.StartsWith(header));
    if (start == -1)
    {
        Console.Error.WriteLine("Version not found.");
        return;
    }

    int end = Array.FindIndex(
        lines,
        start + 1,
        l => l.StartsWith("## [")
    );

    if (end == -1)
        end = lines.Length;

    var resultLines = lines
        .Skip(start + 1)
        .Take(end - start - 1)
        .SkipWhile(string.IsNullOrWhiteSpace);

    File.WriteAllText(
        dstFilePath!.FullName,
        string.Join(Environment.NewLine, resultLines)
    );
});

return extractCommand.Parse(args).Invoke();

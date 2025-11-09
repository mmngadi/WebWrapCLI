# Stage 1: Build environment
FROM mcr.microsoft.com/vs-buildtools:latest AS build

WORKDIR /src

# Copy project files
COPY . .

# Install NuGet CLI
RUN curl -L https://dist.nuget.org/win-x86-commandline/latest/nuget.exe -o C:\\nuget.exe

# Restore packages
RUN C:\\nuget.exe restore WebWrapCLI.vcxproj -PackagesDirectory ..\\packages

# Build project (Release, x64)
RUN msbuild WebWrapCLI.vcxproj /p:Configuration=Release /p:Platform=x64

# Stage 2: Artifact container
FROM mcr.microsoft.com/windows/servercore:ltsc2022 AS artifact

WORKDIR /output

# Copy only the build results
COPY --from=build /src/x64/Release .

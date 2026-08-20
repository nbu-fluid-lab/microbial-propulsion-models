function calculate_veff_chlamydomonas(rootPath)
%CALCULATE_VEFF_CHLAMYDOMONAS Calculate effective velocity for Chlamydomonas.
%   CALCULATE_VEFF_CHLAMYDOMONAS(ROOT_PATH) processes CSV pairs in ROOT_PATH
%   and its immediate child directories. The current directory is used when
%   ROOT_PATH is omitted.

if nargin < 1 || isempty(rootPath)
    rootPath = pwd;
end

if ~(ischar(rootPath) || (isstring(rootPath) && isscalar(rootPath)))
    error('rootPath must be a character vector or a string scalar.');
end

rootPath = char(rootPath);
if ~isfolder(rootPath)
    error('Data root does not exist: %s', rootPath);
end

timeStep = 0.25;

smoothWindow = 3;
minSegmentLength = 2;
minStraightPoints = 2;

straightAngleDeg = 45;
speedPercentile = 50;

maxTransSpeed = 150;

straightAngleRad = straightAngleDeg * pi / 180;

resultVarNames = { ...
    'Folder', ...
    'Circle_File', ...
    'Kinematics_File', ...
    'Circle_Row', ...
    'Video_ID', ...
    'Idx', ...
    'Enter_Frame', ...
    'Exit_Frame', ...
    'Enter_Time', ...
    'Exit_Time', ...
    'Inside_Duration', ...
    'Inside_Distance', ...
    'Matched_Frame_Start', ...
    'Matched_Frame_End', ...
    'Matched_Point_Number', ...
    'Segment_Mean_Speed', ...
    'Translational_Speed', ...
    'Forward_Point_Number', ...
    'Veff', ...
    'Method'};

summaryVarNames = { ...
    'Folder', ...
    'Total_Circle_Row_Number', ...
    'Valid_Veff_Number', ...
    'Skipped_Row_Number', ...
    'Mean_Veff', ...
    'STD_Veff'};

skipVarNames = { ...
    'Folder', ...
    'Circle_File', ...
    'Kinematics_File', ...
    'Circle_Row', ...
    'Video_ID', ...
    'Idx', ...
    'Enter_Frame', ...
    'Exit_Frame', ...
    'Skip_Reason'};

targetFolders = struct('name', {}, 'path', {});

[~, rootName] = fileparts(rootPath);
targetFolders(1).name = rootName;
targetFolders(1).path = rootPath;

folderInfo = dir(rootPath);
folderInfo = folderInfo([folderInfo.isdir]);
folderInfo = folderInfo(~ismember({folderInfo.name}, {'.', '..'}));

for i = 1:length(folderInfo)
    targetFolders(end+1).name = folderInfo(i).name;
    targetFolders(end).path = fullfile(rootPath, folderInfo(i).name);
end

allRows = {};
allSummaryRows = {};
allSkipRows = {};

for f = 1:length(targetFolders)

    folderName = targetFolders(f).name;
    folderPath = targetFolders(f).path;

    csvFiles = dir(fullfile(folderPath, '*.csv'));

    if isempty(csvFiles)
        continue;
    end

    fileNames = string({csvFiles.name});

    resultMask = startsWith(fileNames, "Veff_") | startsWith(fileNames, "All_Folders_");
    csvFiles = csvFiles(~resultMask);
    fileNames = string({csvFiles.name});

    circleFiles = csvFiles(contains(lower(fileNames), "circle"));
    kinFiles = csvFiles(contains(lower(fileNames), "kinematics"));

    if isempty(circleFiles) || isempty(kinFiles)
        continue;
    end

    fprintf('\nProcessing folder: %s\n', folderName);

    folderRows = {};
    folderSkipRows = {};
    folderVeff = [];

    totalCircleRowCount = 0;
    validVeffCount = 0;
    skippedRowCount = 0;

    for cf = 1:length(circleFiles)

        circleName = circleFiles(cf).name;
        circlePath = fullfile(folderPath, circleName);

        prefix = getFilePrefix(circleName);

        if strlength(prefix) == 0
            skippedRowCount = skippedRowCount + 1;
            folderSkipRows = [folderSkipRows; ...
                {folderName, circleName, "", NaN, NaN, NaN, NaN, NaN, "Failed to identify the circle-file prefix"}];
            continue;
        end

        kinNames = string({kinFiles.name});
        matchID = find(startsWith(kinNames, prefix) & contains(lower(kinNames), "kinematics"), 1);

        if isempty(matchID)
            skippedRowCount = skippedRowCount + 1;
            folderSkipRows = [folderSkipRows; ...
                {folderName, circleName, "", NaN, NaN, NaN, NaN, NaN, "No matching kinematics file was found"}];
            continue;
        end

        kinName = kinFiles(matchID).name;
        kinPath = fullfile(folderPath, kinName);

        fprintf('  Matched files: %s  -->  %s\n', circleName, kinName);

        try
            C = readtable(circlePath);
        catch
            skippedRowCount = skippedRowCount + 1;
            folderSkipRows = [folderSkipRows; ...
                {folderName, circleName, kinName, NaN, NaN, NaN, NaN, NaN, "Failed to read circle file"}];
            continue;
        end

        try
            K = readtable(kinPath);
        catch
            skippedRowCount = skippedRowCount + height(C);
            folderSkipRows = [folderSkipRows; ...
                {folderName, circleName, kinName, NaN, NaN, NaN, NaN, NaN, "Failed to read kinematics file"}];
            continue;
        end

        if width(C) < 15
            skippedRowCount = skippedRowCount + height(C);
            folderSkipRows = [folderSkipRows; ...
                {folderName, circleName, kinName, NaN, NaN, NaN, NaN, NaN, "Circle file contains fewer than 15 columns"}];
            continue;
        end

        if width(K) < 9
            skippedRowCount = skippedRowCount + height(C);
            folderSkipRows = [folderSkipRows; ...
                {folderName, circleName, kinName, NaN, NaN, NaN, NaN, NaN, "Kinematics file contains fewer than 9 columns"}];
            continue;
        end

        c_video_id = getNumericColumn(C, 1);
        c_idx = getNumericColumn(C, 2);
        c_enter_frame = getNumericColumn(C, 3);
        c_exit_frame = getNumericColumn(C, 4);
        c_enter_time = getNumericColumn(C, 5);
        c_exit_time = getNumericColumn(C, 6);
        c_inside_duration = getNumericColumn(C, 13);
        c_inside_distance = getNumericColumn(C, 15);

        k_video_id = getNumericColumn(K, 1);
        k_idx = getNumericColumn(K, 2);
        k_frame = getNumericColumn(K, 3);
        k_time = getNumericColumn(K, 4);
        k_angle = getNumericColumn(K, 7);
        k_speed = getNumericColumn(K, 9);

        for r = 1:height(C)

            totalCircleRowCount = totalCircleRowCount + 1;

            videoID = c_video_id(r);
            thisIdx = c_idx(r);
            enterFrame = c_enter_frame(r);
            exitFrame = c_exit_frame(r);
            enterTime = c_enter_time(r);
            exitTime = c_exit_time(r);
            insideDuration = c_inside_duration(r);
            insideDistance = c_inside_distance(r);

            if ~isfinite(thisIdx) || ~isfinite(enterFrame) || ~isfinite(exitFrame)
                skippedRowCount = skippedRowCount + 1;
                folderSkipRows = [folderSkipRows; ...
                    {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, exitFrame, "Invalid idx or frame value in circle row"}];
                continue;
            end

            if exitFrame < enterFrame
                temp = enterFrame;
                enterFrame = exitFrame;
                exitFrame = temp;
            end

            if ~isfinite(insideDuration) || insideDuration <= 0
                insideDuration = (exitFrame - enterFrame) * timeStep;
            end

            if ~isfinite(insideDuration) || insideDuration <= 0
                insideDuration = exitTime - enterTime;
            end

            if ~isfinite(insideDuration) || insideDuration <= 0
                skippedRowCount = skippedRowCount + 1;
                folderSkipRows = [folderSkipRows; ...
                    {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, exitFrame, "Invalid residence time"}];
                continue;
            end

            if ~isfinite(insideDistance) || insideDistance <= 0
                skippedRowCount = skippedRowCount + 1;
                folderSkipRows = [folderSkipRows; ...
                    {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, exitFrame, "Invalid travel distance"}];
                continue;
            end

            segMask = k_idx == thisIdx & ...
                      k_frame >= enterFrame & ...
                      k_frame <= exitFrame & ...
                      isfinite(k_angle) & ...
                      isfinite(k_speed) & ...
                      k_speed >= 0;

            if isfinite(videoID) && any(isfinite(k_video_id))
                segMask = segMask & k_video_id == videoID;
            end

            if sum(segMask) < minSegmentLength && isfinite(enterTime) && isfinite(exitTime)
                timeTol = timeStep / 2;

                segMask = k_idx == thisIdx & ...
                          k_time >= enterTime - timeTol & ...
                          k_time <= exitTime + timeTol & ...
                          isfinite(k_angle) & ...
                          isfinite(k_speed) & ...
                          k_speed >= 0;

                if isfinite(videoID) && any(isfinite(k_video_id))
                    segMask = segMask & k_video_id == videoID;
                end
            end

            segFrame = k_frame(segMask);
            segTime = k_time(segMask);
            segAngle = k_angle(segMask);
            segSpeed = k_speed(segMask);

            if length(segSpeed) < minSegmentLength
                skippedRowCount = skippedRowCount + 1;
                folderSkipRows = [folderSkipRows; ...
                    {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, exitFrame, "Insufficient points in the matching kinematics segment"}];
                continue;
            end

            [segFrame, order] = sort(segFrame);
            segTime = segTime(order);
            segAngle = segAngle(order);
            segSpeed = segSpeed(order);

            [segFrame, uniqueID] = unique(segFrame, 'stable');
            segTime = segTime(uniqueID);
            segAngle = segAngle(uniqueID);
            segSpeed = segSpeed(uniqueID);

            if length(segSpeed) < minSegmentLength
                skippedRowCount = skippedRowCount + 1;
                folderSkipRows = [folderSkipRows; ...
                    {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, exitFrame, "Insufficient points in the trajectory segment after duplicate removal"}];
                continue;
            end

            [transSpeed, nForwardPoints, segMeanSpeed, method] = ...
                calcChlamyShortTransSpeed(segSpeed, segAngle, ...
                smoothWindow, straightAngleRad, speedPercentile, minStraightPoints);

            if ~isfinite(transSpeed) || transSpeed <= 0
                skippedRowCount = skippedRowCount + 1;
                folderSkipRows = [folderSkipRows; ...
                    {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, exitFrame, "Translational speed could not be calculated"}];
                continue;
            end

            if transSpeed > maxTransSpeed
                skippedRowCount = skippedRowCount + 1;
                folderSkipRows = [folderSkipRows; ...
                    {folderName, circleName, kinName, r, videoID, thisIdx, enterFrame, exitFrame, "Translational speed exceeds 150"}];
                continue;
            end

            Veff = insideDistance / insideDuration / transSpeed;

            validVeffCount = validVeffCount + 1;
            folderVeff = [folderVeff; Veff];

            rowData = { ...
                folderName, ...
                circleName, ...
                kinName, ...
                r, ...
                videoID, ...
                thisIdx, ...
                enterFrame, ...
                exitFrame, ...
                enterTime, ...
                exitTime, ...
                insideDuration, ...
                insideDistance, ...
                segFrame(1), ...
                segFrame(end), ...
                length(segSpeed), ...
                segMeanSpeed, ...
                transSpeed, ...
                nForwardPoints, ...
                Veff, ...
                string(method)};

            folderRows = [folderRows; rowData];
            allRows = [allRows; rowData];

        end
    end

    folderResult = safeCell2Table(folderRows, resultVarNames);
    folderSkipResult = safeCell2Table(folderSkipRows, skipVarNames);

    meanVeff = meanFinite(folderVeff);
    stdVeff = stdFinite(folderVeff);

    folderSummaryRows = { ...
        folderName, ...
        totalCircleRowCount, ...
        validVeffCount, ...
        skippedRowCount, ...
        meanVeff, ...
        stdVeff};

    folderSummary = safeCell2Table(folderSummaryRows, summaryVarNames);

    allSummaryRows = [allSummaryRows; folderSummaryRows];
    allSkipRows = [allSkipRows; folderSkipRows];

    writetable(folderResult, fullfile(folderPath, 'Veff_All_Rows_In_This_Folder.csv'));
    writetable(folderSummary, fullfile(folderPath, 'Veff_Folder_Summary.csv'));
    writetable(folderSkipResult, fullfile(folderPath, 'Veff_Skipped_Rows.csv'));

    fprintf('  Folder %s completed: valid Veff = %d, skipped = %d, mean Veff = %.6f, SD = %.6f\n', ...
        folderName, validVeffCount, skippedRowCount, meanVeff, stdVeff);

end

allResult = safeCell2Table(allRows, resultVarNames);
summaryResult = safeCell2Table(allSummaryRows, summaryVarNames);
skipResult = safeCell2Table(allSkipRows, skipVarNames);

writetable(allResult, fullfile(rootPath, 'All_Folders_All_Veff.csv'));
writetable(summaryResult, fullfile(rootPath, 'All_Folders_Veff_Summary.csv'));
writetable(skipResult, fullfile(rootPath, 'All_Folders_Skipped_Rows.csv'));

outXlsx = fullfile(rootPath, 'All_Folders_Veff_Result.xlsx');

writetable(allResult, outXlsx, 'Sheet', 'All_Veff');
writetable(summaryResult, outXlsx, 'Sheet', 'Summary_By_Folder');
writetable(skipResult, outXlsx, 'Sheet', 'Skipped_Rows');

disp('All calculations have been completed.');
disp(fullfile(rootPath, 'All_Folders_All_Veff.csv'));
disp(fullfile(rootPath, 'All_Folders_Veff_Summary.csv'));
disp(fullfile(rootPath, 'All_Folders_Skipped_Rows.csv'));
disp(fullfile(rootPath, 'All_Folders_Veff_Result.xlsx'));

end

function [transSpeed, nForwardPoints, segMeanSpeed, method] = ...
    calcChlamyShortTransSpeed(speed, angle, smoothWindow, straightAngleRad, speedPercentile, minStraightPoints)

    speed = speed(:);
    angle = angle(:);

    valid = isfinite(speed) & isfinite(angle) & speed >= 0;
    speed = speed(valid);
    angle = angle(valid);

    transSpeed = NaN;
    nForwardPoints = 0;
    segMeanSpeed = NaN;
    method = "invalid";

    if length(speed) < 2
        method = "too_few_points";
        return;
    end

    win = min(smoothWindow, length(speed));
    smoothSpeed = movmean(speed, win);

    segMeanSpeed = meanFinite(smoothSpeed);

    if length(smoothSpeed) == 2
        transSpeed = segMeanSpeed;
        nForwardPoints = 2;
        method = "two_points_use_segment_mean_speed";
        return;
    end

    dtheta = [0; abs(atan2(sin(diff(angle)), cos(diff(angle))))];
    smoothDtheta = movmean(dtheta, win);

    valid2 = isfinite(smoothSpeed) & isfinite(smoothDtheta);
    smoothSpeed = smoothSpeed(valid2);
    smoothDtheta = smoothDtheta(valid2);

    if length(smoothSpeed) < 2
        transSpeed = segMeanSpeed;
        nForwardPoints = length(speed);
        method = "fallback_use_segment_mean_speed";
        return;
    end

    angleMask = smoothDtheta <= straightAngleRad;

    speedThreshold = percentileNoToolbox(smoothSpeed, speedPercentile);

    forwardMask = angleMask & smoothSpeed >= speedThreshold;
    forwardMask = keepLongContinuousSegments(forwardMask, minStraightPoints);

    forwardSpeed = smoothSpeed(forwardMask);

    if length(forwardSpeed) >= minStraightPoints
        transSpeed = meanFinite(forwardSpeed);
        nForwardPoints = length(forwardSpeed);
        method = "straight_and_high_speed";
        return;
    end

    straightSpeed = smoothSpeed(angleMask);

    if length(straightSpeed) >= minStraightPoints
        transSpeed = meanFinite(straightSpeed);
        nForwardPoints = length(straightSpeed);
        method = "straight_points_mean_speed";
        return;
    end

    transSpeed = segMeanSpeed;
    nForwardPoints = length(smoothSpeed);
    method = "short_residence_use_segment_mean_speed";

end

function x = getNumericColumn(T, colIndex)

    x = T{:, colIndex};

    if iscell(x)
        x = str2double(x);
    elseif isstring(x)
        x = str2double(x);
    elseif ischar(x)
        x = str2double(cellstr(x));
    else
        x = double(x);
    end

end

function q = percentileNoToolbox(x, p)

    x = x(:);
    x = x(isfinite(x));

    if isempty(x)
        q = NaN;
        return;
    end

    x = sort(x);
    n = length(x);

    if n == 1
        q = x;
        return;
    end

    pos = 1 + (p / 100) * (n - 1);

    lower = floor(pos);
    upper = ceil(pos);

    if lower == upper
        q = x(lower);
    else
        q = x(lower) + (pos - lower) * (x(upper) - x(lower));
    end

end

function maskOut = keepLongContinuousSegments(maskIn, minLen)

    maskIn = maskIn(:);
    maskOut = false(size(maskIn));

    if isempty(maskIn)
        return;
    end

    d = diff([false; maskIn; false]);

    startIdx = find(d == 1);
    endIdx = find(d == -1) - 1;

    for i = 1:length(startIdx)

        segLen = endIdx(i) - startIdx(i) + 1;

        if segLen >= minLen
            maskOut(startIdx(i):endIdx(i)) = true;
        end

    end

end

function prefix = getFilePrefix(fileName)

    token = regexp(char(fileName), '^\d+', 'match', 'once');

    if isempty(token)
        prefix = "";
    else
        prefix = string(token);
    end

end

function m = meanFinite(x)

    x = x(:);
    x = x(isfinite(x));

    if isempty(x)
        m = NaN;
    else
        m = mean(x);
    end

end

function s = stdFinite(x)

    x = x(:);
    x = x(isfinite(x));

    if isempty(x)
        s = NaN;
    elseif length(x) == 1
        s = 0;
    else
        s = std(x);
    end

end

function T = safeCell2Table(rows, varNames)

    nVar = length(varNames);

    if isempty(rows)
        rows = cell(0, nVar);
    end

    if size(rows, 2) ~= nVar
        error('safeCell2Table: rows has %d columns, but VariableNames has %d elements.', ...
            size(rows, 2), nVar);
    end

    T = cell2table(rows, 'VariableNames', varNames);

end

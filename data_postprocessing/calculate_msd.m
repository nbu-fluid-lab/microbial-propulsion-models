function calculate_msd(dataDir)
%CALCULATE_MSD Calculate mean-squared displacement from trajectory CSV files.
%   CALCULATE_MSD(DATA_DIR) processes all CSV files directly inside DATA_DIR.
%   When DATA_DIR is omitted, the current working directory is used.

if nargin < 1 || isempty(dataDir)
    dataDir = pwd;
end

if ~(ischar(dataDir) || (isstring(dataDir) && isscalar(dataDir)))
    error('dataDir must be a character vector or a string scalar.');
end

dataDir = char(dataDir);
if ~isfolder(dataDir)
    error('Data directory does not exist: %s', dataDir);
end

outDir = fullfile(dataDir, 'MSD_raw_speedFiltered_longTime_output');

if ~exist(outDir, 'dir')
    mkdir(outDir);
end

minAverageSpeed = 200;
durationRatio = 0.90;
minSelectedTracks = 3;
minPoints = 10;
minTracksPerLag = 1;

csvFiles = dir(fullfile(dataDir, '*.csv'));

if isempty(csvFiles)
    error('No CSV files were found in the supplied data directory.');
end

allTracksCell = {};
allCandidatesCell = {};
globalTrackID = 0;

for f = 1:length(csvFiles)

    fileName = csvFiles(f).name;
    filePath = fullfile(dataDir, fileName);

    fprintf('\nProcessing file: %s\n', fileName);

    T = readtable(filePath, 'VariableNamingRule', 'preserve');

    if width(T) < 5
        warning('File %s has insufficient columns and was skipped.', fileName);
        continue;
    end

    vars = string(T.Properties.VariableNames);

    videoCol = findOptionalColumn(vars, ["video_id", "video", "movie_id"], []);
    frameCol = findColumn(vars, ["frame", "frames"], 2);
    timeCol  = findColumn(vars, ["time", "t"], 3);
    idxCol   = findColumn(vars, ["idx", "id", "track_id", "particle_id"], 4);
    xCol     = findColumn(vars, ["x", "x_px", "pos_x", "x_position"], 5);
    yCol     = findColumn(vars, ["y", "y_px", "pos_y", "y_position"], 6);

    requiredColumns = [frameCol, timeCol, idxCol, xCol, yCol];
    if any(requiredColumns < 1 | requiredColumns > width(T))
        warning('File %s does not contain all required trajectory columns and was skipped.', fileName);
        continue;
    end

    frame = round(toNumeric(T{:, frameCol}));
    time  = toNumeric(T{:, timeCol});
    idx   = string(T{:, idxCol});

    x = toNumeric(T{:, xCol});
    y = toNumeric(T{:, yCol});

    if isempty(videoCol)
        videoID = repmat(string(fileName), height(T), 1);
    else
        videoID = string(T{:, videoCol});
    end

    valid = ~isnan(frame) & ~isnan(time) & ~isnan(x) & ~isnan(y) ...
            & ~ismissing(idx) & idx ~= "";

    frame = frame(valid);
    time = time(valid);
    idx = idx(valid);
    x = x(valid);
    y = y(valid);
    videoID = videoID(valid);

    if isempty(time)
        warning('File %s contains no valid data and was skipped.', fileName);
        continue;
    end

    dtFrame = estimateFrameInterval(frame, time);
    fprintf('  Estimated single-frame interval, dtFrame = %.6f s\n', dtFrame);

    [G, groupVideo, groupIdx] = findgroups(videoID, idx);

    tracks = struct([]);
    candidates = struct([]);

    for g = 1:max(G)

        ind = find(G == g);

        fg = frame(ind);
        tg = time(ind);
        xg = x(ind);
        yg = y(ind);

        [fg, order] = sort(fg);
        tg = tg(order);
        xg = xg(order);
        yg = yg(order);

        [fUnique, ~, ic] = unique(fg, 'stable');

        tUnique = accumarray(ic, tg, [], @mean);
        xUnique = accumarray(ic, xg, [], @mean);
        yUnique = accumarray(ic, yg, [], @mean);

        fg = fUnique;
        tg = tUnique;
        xg = xUnique;
        yg = yUnique;

        if length(fg) < minPoints
            continue;
        end

        duration = tg(end) - tg(1);
        frameSpan = fg(end) - fg(1);

        if duration <= 0 || frameSpan <= 0
            continue;
        end

        dxStep = diff(xg);
        dyStep = diff(yg);
        stepDist = sqrt(dxStep.^2 + dyStep.^2);

        stepDist = stepDist(~isnan(stepDist));
        pathLength = sum(stepDist);

        avgSpeed = pathLength / duration;

        globalTrackID = globalTrackID + 1;

        cand.trackID = globalTrackID;
        cand.fileName = char(fileName);
        cand.videoID = char(groupVideo(g));
        cand.particleID = char(groupIdx(g));
        cand.nPoints = length(fg);
        cand.frameStart = fg(1);
        cand.frameEnd = fg(end);
        cand.frameSpan = frameSpan;
        cand.tStart = tg(1);
        cand.tEnd = tg(end);
        cand.duration = duration;
        cand.pathLength = pathLength;
        cand.avgSpeed = avgSpeed;
        cand.minAverageSpeed = minAverageSpeed;
        cand.passSpeedFilter = avgSpeed >= minAverageSpeed;

        if isempty(candidates)
            candidates = cand;
        else
            candidates(end+1, 1) = cand;
        end

        if avgSpeed < minAverageSpeed
            continue;
        end

        [frameLag, tau, msd, nPairs] = calculateMSD_byFrameLag_raw( ...
            fg, xg, yg, dtFrame);

        if isempty(msd) || all(isnan(msd))
            continue;
        end

        temp.trackID = globalTrackID;
        temp.fileName = char(fileName);
        temp.videoID = char(groupVideo(g));
        temp.particleID = char(groupIdx(g));
        temp.nPoints = length(fg);
        temp.frameStart = fg(1);
        temp.frameEnd = fg(end);
        temp.frameSpan = frameSpan;
        temp.tStart = tg(1);
        temp.tEnd = tg(end);
        temp.duration = duration;
        temp.pathLength = pathLength;
        temp.avgSpeed = avgSpeed;
        temp.dtFrame = dtFrame;
        temp.frameLag = frameLag;
        temp.tau = tau;
        temp.msd = msd;
        temp.nPairs = nPairs;

        if isempty(tracks)
            tracks = temp;
        else
            tracks(end+1, 1) = temp;
        end

    end

    if isempty(candidates)
        warning('File %s contains no trajectories satisfying the minimum point-count and duration requirements.', fileName);
    else
        allCandidatesCell{end+1} = candidates;
    end

    if isempty(tracks)
        warning('File %s contains no valid trajectories with an average speed >= %.2f.', fileName, minAverageSpeed);
        continue;
    end

    fprintf('  Number of valid trajectories after speed filtering: %d\n', length(tracks));

    allTracksCell{end+1} = tracks;

end

if isempty(allCandidatesCell)
    error('No candidate trajectories satisfying the basic requirements were found.');
end

allCandidates = vertcat(allCandidatesCell{:});

if isempty(allTracksCell)
    error('The average speeds of all trajectories are below %.2f; MSD cannot be calculated.', minAverageSpeed);
end

allTracks = vertcat(allTracksCell{:});

durations = [allTracks.duration]';
maxDuration = max(durations);

selected = durations >= durationRatio * maxDuration;

if sum(selected) < minSelectedTracks
    numToSelect = min(minSelectedTracks, length(allTracks));
    [~, sortID] = sort(durations, 'descend');
    selected(:) = false;
    selected(sortID(1:numToSelect)) = true;
end

selectedTracks = allTracks(selected);
selectedTrackIDs = [selectedTracks.trackID]';

fprintf('\n================ Global Trajectory Selection Results ================\n');
fprintf('Number of candidate trajectories: %d\n', length(allCandidates));
fprintf('Number of trajectories with an average speed >= %.2f: %d\n', minAverageSpeed, length(allTracks));
fprintf('Number of long-duration trajectories used to calculate the mean MSD: %d\n', length(selectedTracks));
fprintf('Maximum duration after speed filtering: %.6f s\n', maxDuration);
fprintf('Selection criterion: duration >= %.2f x maxDuration = %.6f s\n', ...
    durationRatio, durationRatio * maxDuration);

candidateTrackIDs = [allCandidates.trackID]';

summaryTable = table;
summaryTable.trackID = candidateTrackIDs;
summaryTable.fileName = string({allCandidates.fileName})';
summaryTable.videoID = string({allCandidates.videoID})';
summaryTable.particleID = string({allCandidates.particleID})';
summaryTable.nPoints = [allCandidates.nPoints]';
summaryTable.frameStart = [allCandidates.frameStart]';
summaryTable.frameEnd = [allCandidates.frameEnd]';
summaryTable.frameSpan = [allCandidates.frameSpan]';
summaryTable.tStart = [allCandidates.tStart]';
summaryTable.tEnd = [allCandidates.tEnd]';
summaryTable.duration = [allCandidates.duration]';
summaryTable.pathLength = [allCandidates.pathLength]';
summaryTable.avgSpeed = [allCandidates.avgSpeed]';
summaryTable.minAverageSpeed = [allCandidates.minAverageSpeed]';
summaryTable.passSpeedFilter = [allCandidates.passSpeedFilter]';
summaryTable.selectedForAverageMSD = ismember(candidateTrackIDs, selectedTrackIDs);

summaryOut = fullfile(outDir, 'ALL_raw_speedFiltered_track_summary.csv');
writetable(summaryTable, summaryOut);

fprintf('\nTrajectory summary table saved to: %s\n', summaryOut);

avgMSDTable = averageMSD_byFrameLag_longTime(selectedTracks, minTracksPerLag);

avgOut = fullfile(outDir, 'ALL_raw_speedFiltered_longTime_average_MSD.csv');
writetable(avgMSDTable, avgOut);

fprintf('\nMean MSD data saved to: %s\n', avgOut);

if ~isempty(avgMSDTable)
    fprintf('Initial tau value of the mean MSD: %.6f s\n', avgMSDTable.tau(1));
    fprintf('Maximum tau value of the mean MSD: %.6f s\n', max(avgMSDTable.tau));
    fprintf('Number of data points in the mean MSD: %d\n', height(avgMSDTable));
else
    warning('The mean MSD table is empty. Please check the trajectory data.');
end

fprintf('\nProcessing completed. Results were saved in: %s\n', outDir);

end

function col = findColumn(vars, candidates, fallbackCol)

    varsLower = lower(strtrim(vars));
    candidatesLower = lower(strtrim(candidates));

    col = [];

    for i = 1:length(candidatesLower)
        hit = find(varsLower == candidatesLower(i), 1);
        if ~isempty(hit)
            col = hit;
            return;
        end
    end

    col = fallbackCol;

end

function col = findOptionalColumn(vars, candidates, defaultValue)

    varsLower = lower(strtrim(vars));
    candidatesLower = lower(strtrim(candidates));

    col = defaultValue;

    for i = 1:length(candidatesLower)
        hit = find(varsLower == candidatesLower(i), 1);
        if ~isempty(hit)
            col = hit;
            return;
        end
    end

end

function x = toNumeric(x)

    if isnumeric(x)
        x = double(x);
    elseif iscell(x)
        x = str2double(string(x));
    elseif isstring(x)
        x = str2double(x);
    elseif iscategorical(x)
        x = str2double(string(x));
    else
        x = str2double(string(x));
    end

end

function dtFrame = estimateFrameInterval(frame, time)

    frame = round(frame(:));
    time = time(:);

    valid = ~isnan(frame) & ~isnan(time);
    frame = frame(valid);
    time = time(valid);

    [frameSorted, order] = sort(frame);
    timeSorted = time(order);

    [fUnique, ~, ic] = unique(frameSorted, 'stable');
    tUnique = accumarray(ic, timeSorted, [], @mean);

    df = diff(fUnique);
    dt = diff(tUnique);

    validDiff = df > 0 & dt > 0;

    if any(validDiff)
        dtFrame = median(dt(validDiff) ./ df(validDiff), 'omitnan');
    else
        tUnique2 = unique(time);
        dtList = diff(tUnique2);
        dtList = dtList(dtList > 0);

        if isempty(dtList)
            error('The single-frame interval could not be estimated. Please check the frame and time columns.');
        else
            dtFrame = median(dtList, 'omitnan');
        end
    end

end

function [frameLag, tau, msd, nPairs] = calculateMSD_byFrameLag_raw(frame, x, y, dtFrame)

    frame = round(frame(:));
    x = x(:);
    y = y(:);

    maxFrameLag = frame(end) - frame(1);

    frameLag = (1:maxFrameLag)';
    tau = frameLag * dtFrame;
    msd = nan(maxFrameLag, 1);
    nPairs = zeros(maxFrameLag, 1);

    for lag = 1:maxFrameLag

        [hasPair, locFuture] = ismember(frame + lag, frame);

        idxNow = find(hasPair);
        idxFuture = locFuture(hasPair);

        if isempty(idxNow)
            continue;
        end

        dx = x(idxFuture) - x(idxNow);
        dy = y(idxFuture) - y(idxNow);

        dr2 = dx.^2 + dy.^2;

        valid = ~isnan(dr2);

        if any(valid)
            msd(lag) = mean(dr2(valid), 'omitnan');
            nPairs(lag) = sum(valid);
        end

    end

end

function avgTable = averageMSD_byFrameLag_longTime(tracks, minTracksPerLag)

    if nargin < 2
        minTracksPerLag = 1;
    end

    nTracks = length(tracks);

    if nTracks == 0
        avgTable = table;
        return;
    end

    maxFrameLagAll = 0;

    for i = 1:nTracks
        maxFrameLagAll = max(maxFrameLagAll, max(tracks(i).frameLag));
    end

    tauMat = nan(maxFrameLagAll, nTracks);
    msdMat = nan(maxFrameLagAll, nTracks);
    pairMat = zeros(maxFrameLagAll, nTracks);

    for i = 1:nTracks

        frameLag_i = tracks(i).frameLag;
        tau_i = tracks(i).tau;
        msd_i = tracks(i).msd;
        pair_i = tracks(i).nPairs;

        valid = frameLag_i >= 1 & frameLag_i <= maxFrameLagAll;

        tauMat(frameLag_i(valid), i) = tau_i(valid);
        msdMat(frameLag_i(valid), i) = msd_i(valid);
        pairMat(frameLag_i(valid), i) = pair_i(valid);

    end

    frameLagOut = (1:maxFrameLagAll)';

    tauMean = mean(tauMat, 2, 'omitnan');
    msdMean = mean(msdMat, 2, 'omitnan');
    msdStd = std(msdMat, 0, 2, 'omitnan');

    nUsedTracks = sum(~isnan(msdMat), 2);
    nUsedPairs = sum(pairMat, 2);

    validLag = nUsedTracks >= minTracksPerLag & nUsedPairs > 0 & ~isnan(msdMean);

    avgTable = table;
    avgTable.frameLag = frameLagOut(validLag);
    avgTable.tau = tauMean(validLag);
    avgTable.meanMSD = msdMean(validLag);
    avgTable.stdMSD = msdStd(validLag);
    avgTable.nUsedTracks = nUsedTracks(validLag);
    avgTable.nUsedPairs = nUsedPairs(validLag);

end
